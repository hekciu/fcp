#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <pthread.h>
#include <libaio.h>

#include "copy.h"
#include "common.h"
#include "timer.h"

typedef struct {
	int src_fd;
	int dest_fd;
    size_t offset;
    size_t n_bytes;
	uint32_t queue_depth;
	size_t fs_block_size;
	int thread_num;
} copy_thread_params_t;

typedef struct {
	size_t n_bytes;
	bool was_read_finished;
} async_copy_item_t;

static FCP_ERROR assert_file_type(struct stat* sb);
static FCP_ERROR get_file_size(struct stat* sb, size_t* out);

static void* sync_copy_thread_callback(void* copy_thread_params);
static void* async_copy_thread_callback(void* copy_thread_params);

static struct iocb*** write_configs_ptrs_array = NULL;
static struct iocb*** read_configs_ptrs_array = NULL;

static io_context_t* io_context_read_array = 0;
static io_context_t* io_context_write_array = 0;


FCP_ERROR fcp_copy(fcp_copy_config_t* config, fcp_copy_output_t* output) {
    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;
    read_args |= O_DIRECT; // disable kernel caching

    write_args = O_WRONLY;
    write_args |= O_CREAT;
	write_args |= O_DIRECT;
    // write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE("open (read)", (src_fd = open(config->src, read_args)));
    mode_t write_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    SYSCALL_ERR_HANDLE("open (write)", (dest_fd = open(config->dest, write_args, write_mode)));

    struct stat src_sb = {0};
    SYSCALL_ERR_HANDLE("fstat", fstat(src_fd, &src_sb));

    HANDLE_ERROR(assert_file_type(&src_sb));

    size_t src_size = 0;

    HANDLE_ERROR(get_file_size(&src_sb, &src_size));

    pthread_t* threads = malloc(sizeof(pthread_t) * config->threads);
    copy_thread_params_t* threads_params = malloc(sizeof(copy_thread_params_t) * config->threads);

	/* We essentially do not care about memory leaks */

	write_configs_ptrs_array = malloc(config->threads * sizeof(struct iocb**));
	read_configs_ptrs_array = malloc(config->threads * sizeof(struct iocb**));

	io_context_read_array = malloc(config->threads * sizeof(io_context_t*));
	io_context_write_array = malloc(config->threads * sizeof(io_context_t*));

    size_t bytes_per_section = (src_size / config->threads);

    /* timer start */
    fcp_timer_t timer;
    start_timer(&timer);

    for (size_t t_num = 0; t_num < config->threads; t_num++) {
        copy_thread_params_t* params = threads_params + t_num;
        pthread_t* thread = threads + t_num;

        size_t bytes_left = src_size - (t_num * bytes_per_section);

        size_t copy_bytes = bytes_left > bytes_per_section ? bytes_per_section : bytes_left;

        size_t offset = t_num * bytes_per_section;

        params->src_fd = src_fd;
        params->dest_fd = dest_fd;
        params->offset = offset;
        params->n_bytes = copy_bytes;
        params->queue_depth = config->queue_depth;
        params->fs_block_size = config->fs_block_size;
		params->thread_num = t_num;

		if (config->async) {
			SYSCALL_ERR_HANDLE("pthread_create (sync)", pthread_create(thread,
						   NULL, 
						   async_copy_thread_callback,
						   (void*)params));
		} else {
			SYSCALL_ERR_HANDLE("pthread_create (sync)", pthread_create(thread,
						   NULL, 
						   sync_copy_thread_callback,
						   (void*)params));
		}

    }

    for (pthread_t* thread = threads; thread < (threads + config->threads); thread++) {
        pthread_join(*thread, NULL);
    }

    stop_timer(&timer);
    output->elapsed_ns = timer.elapsed_ns;

    free(threads);
    free(threads_params);

    return FCP_OK;
}


static void* async_copy_thread_callback(void* copy_thread_params) {
    copy_thread_params_t* params = (copy_thread_params_t*) copy_thread_params;

	uint8_t* copy_buffer = NULL;

	/* TODO: memory leak below! */
	SYSCALL_ERR_HANDLE_PTHREAD("posix_memalign", posix_memalign((void**)&copy_buffer, params->fs_block_size, params->n_bytes));

	int maxevents = (int)params->queue_depth; // TODO: Casting from uint32_t to int, change queue_depth param to be int from the beginning

	SYSCALL_ERR_HANDLE_PTHREAD_LIBAIO("io_setup (io_context_read)", io_setup(maxevents, (io_context_read_array + params->thread_num)));
	SYSCALL_ERR_HANDLE_PTHREAD_LIBAIO("io_setup (io_context_write)", io_setup(maxevents, (io_context_write_array + params->thread_num)));

	/* TODO: memory leak below! */
	struct iocb* read_configs = calloc(maxevents, sizeof(struct iocb));
	/* TODO: memory leak below! */
	struct iocb* write_configs = calloc(maxevents, sizeof(struct iocb));

	/* TODO: memory leak below! */
	struct io_event* read_events = calloc(maxevents, sizeof(struct io_event));
	/* TODO: memory leak below! */
	struct io_event* write_events = calloc(maxevents, sizeof(struct io_event));

	size_t bytes_per_call = params->n_bytes / (size_t)maxevents;

	/* TODO: memory leak below! */
	async_copy_item_t* copy_states = calloc(maxevents, sizeof(async_copy_item_t));

	/* TODO: memory leak below! */
	*(write_configs_ptrs_array + params->thread_num) = calloc(maxevents, sizeof(struct iocb*));
	*(read_configs_ptrs_array + params->thread_num) = calloc(maxevents, sizeof(struct iocb*));

	for (int n = 0; n < maxevents; n++) {
		struct iocb* cur_iocb_read = read_configs + n;
		struct iocb* cur_iocb_write = write_configs + n;
		size_t relative_offset = n * bytes_per_call;
		size_t offset = params->offset + relative_offset;
		size_t copy_bytes = (n == (maxevents - 1)) ? (params->n_bytes - relative_offset) : bytes_per_call;

		io_prep_pread(cur_iocb_read, params->src_fd, copy_buffer + relative_offset, copy_bytes, offset);
		io_prep_pwrite(cur_iocb_write, params->dest_fd, copy_buffer + relative_offset, copy_bytes, offset);

		cur_iocb_read->data = (void*)(size_t)n;
		cur_iocb_write->data = (void*)(size_t)n;

		*(*(write_configs_ptrs_array + params->thread_num) + n) = cur_iocb_write;
		*(*(read_configs_ptrs_array + params->thread_num) + n) = cur_iocb_read;
	}

	SYSCALL_ERR_HANDLE_PTHREAD("io_submit (read events)", io_submit(*(io_context_read_array + params->thread_num), maxevents, *(read_configs_ptrs_array + params->thread_num)));

	struct timespec timespec_zeros = {
		.tv_sec = 0,
		.tv_nsec = 0
	};

	size_t read_events_done = 0;

	for(int i = 0; i < maxevents; i++) {
		struct io_event* ev = malloc(sizeof(struct io_event));
		SYSCALL_ERR_HANDLE_PTHREAD("io_getevents (read)", io_getevents(*(io_context_read_array + params->thread_num), 1, 1, ev, 0));

		size_t num_conf = (size_t)ev->data;

		struct iocb* iocb_write = *(*(write_configs_ptrs_array + params->thread_num) + num_conf);

		SYSCALL_ERR_HANDLE_PTHREAD("io_submit (write event)",
			io_submit(*(io_context_write_array + params->thread_num), 1, (*(write_configs_ptrs_array + params->thread_num) + num_conf)));
	}

	io_getevents(*(io_context_write_array + params->thread_num), maxevents, maxevents, write_events, 0);

	SYSCALL_ERR_HANDLE_PTHREAD("io_destroy io_context_read", io_destroy(*(io_context_read_array + params->thread_num)));
	SYSCALL_ERR_HANDLE_PTHREAD("io_destroy io_context_write", io_destroy(*(io_context_write_array + params->thread_num)));
}



static void* sync_copy_thread_callback(void* copy_thread_params) {
    copy_thread_params_t* params = (copy_thread_params_t*) copy_thread_params;

	uint8_t* copy_buffer = NULL;

	/* TODO: memory leak below! */
	SYSCALL_ERR_HANDLE_PTHREAD("posix_memalign", posix_memalign((void**)&copy_buffer, params->fs_block_size, params->n_bytes));

	SYSCALL_ERR_HANDLE_PTHREAD("pread", pread(params->src_fd, copy_buffer, params->n_bytes, params->offset));

	SYSCALL_ERR_HANDLE_PTHREAD("pwrite", pwrite(params->dest_fd, copy_buffer, params->n_bytes, params->offset));

    return (void*)FCP_OK;
}


static FCP_ERROR assert_file_type(struct stat* sb) {
    if ((sb->st_mode & S_IFMT) != S_IFREG) return FCP_BAD_FILE_TYPE;

    return FCP_OK;
}


static FCP_ERROR get_file_size(struct stat* sb, size_t* out) {
    *out = sb->st_size;

    return FCP_OK;
}
