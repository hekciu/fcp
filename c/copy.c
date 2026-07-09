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
    const char* input;
    const char* output;
    size_t offset;
    off_t n_bytes;
	uint32_t queue_depth;
} copy_thread_params_t;

typedef struct {
	size_t n_bytes;
	bool was_read_finished;
} async_copy_item_t;

static FCP_ERROR assert_file_type(struct stat* sb);
static FCP_ERROR get_file_size(struct stat* sb, off_t* out);

static void* sync_copy_thread_callback(void* copy_thread_params);
static void* async_copy_thread_callback(void* copy_thread_params);


FCP_ERROR fcp_copy(fcp_copy_config_t* config, fcp_copy_output_t* output) {
    int src_fd, read_args;

    read_args = O_RDONLY;
    // read_args |= O_DIRECT; // disable kernel caching

    SYSCALL_ERR_HANDLE("open (read)", (src_fd = open(config->src, read_args)));

    struct stat src_sb = {0};
    SYSCALL_ERR_HANDLE("fstat", fstat(src_fd, &src_sb));

    HANDLE_ERROR(assert_file_type(&src_sb));

    off_t src_size = 0;

    HANDLE_ERROR(get_file_size(&src_sb, &src_size));

    pthread_t* threads = malloc(sizeof(pthread_t) * config->threads);
    copy_thread_params_t* threads_params = malloc(sizeof(copy_thread_params_t) * config->threads);

    size_t bytes_per_section = (src_size / config->threads) + 1;

    /* timer start */
    fcp_timer_t timer;
    start_timer(&timer);

    for (size_t t_num = 0; t_num < config->threads; t_num++) {
        copy_thread_params_t* params = threads_params + t_num;
        pthread_t* thread = threads + t_num;

        size_t bytes_left = src_size - (t_num * bytes_per_section);

        size_t copy_bytes = bytes_left > bytes_per_section ? bytes_per_section : bytes_left;

        size_t offset = t_num * bytes_per_section;

        params->input = config->src;
        params->output = config->dest;
        params->offset = offset;
        params->n_bytes = copy_bytes;
        params->queue_depth = config->queue_depth;

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


static void libaio_read_cb(io_context_t ctx, struct iocb *iocb, long res, long res2);


static struct iocb** write_configs_ptrs = NULL;

static io_context_t io_context_read = 0;
static io_context_t io_context_write = 0;


static void* async_copy_thread_callback(void* copy_thread_params) {
    copy_thread_params_t* params = (copy_thread_params_t*) copy_thread_params;

	/* TODO: memory leak below! */
	// uint8_t* copy_buffer = malloc(params->n_bytes);

	uint8_t* copy_buffer = NULL;

	SYSCALL_ERR_HANDLE_PTHREAD("posix_memalign", posix_memalign((void**)&copy_buffer, params->n_bytes, params->n_bytes));

    int read_args, write_args;

    read_args = O_RDONLY;
    read_args |= O_DIRECT; // disable kernel caching

    write_args = O_WRONLY;
    write_args |= O_CREAT;
    write_args |= O_DIRECT; // disable kernel caching
    // write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    mode_t write_mode = S_IRWXU | S_IRWXG | S_IRWXO;

	int maxevents = (int)params->queue_depth; // TODO: Casting from uint32_t to int, change queue_depth param to be int from the beginning

	SYSCALL_ERR_HANDLE_PTHREAD("io_setup io_context_read", io_setup(maxevents, &io_context_read));
	SYSCALL_ERR_HANDLE_PTHREAD("io_setup io_context_write", io_setup(maxevents, &io_context_write));

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
	write_configs_ptrs = calloc(maxevents, sizeof(struct iocb*));

	int src_fd, dest_fd;
	SYSCALL_ERR_HANDLE_PTHREAD("open (read)", (src_fd = open(params->input, read_args)));
	SYSCALL_ERR_HANDLE_PTHREAD("open (write)", (dest_fd = open(params->output, write_args, write_mode)));

	for (int n = 0; n < maxevents; n++) {
		struct iocb* cur_iocb_read = read_configs + n;
		struct iocb* cur_iocb_write = write_configs + n;
		size_t relative_offset = n * bytes_per_call;
		size_t offset = params->offset + relative_offset;
		size_t copy_bytes = (n == (maxevents - 1)) ? (params->n_bytes - relative_offset) : bytes_per_call;

		io_prep_pread(cur_iocb_read, src_fd, copy_buffer + relative_offset, copy_bytes, offset);
		io_prep_pwrite(cur_iocb_write, dest_fd, copy_buffer + relative_offset, copy_bytes, offset);

		io_set_callback(cur_iocb_read, libaio_read_cb);
		*(write_configs_ptrs + n) = cur_iocb_write;

		cur_iocb_read->data = (void*)((int)n);
	}

	SYSCALL_ERR_HANDLE_PTHREAD("io_submit (read events)", io_submit(io_context_read, maxevents, &read_configs));



	io_getevents(io_context_write, maxevents, maxevents, write_events, 0);

	SYSCALL_ERR_HANDLE_PTHREAD("io_destroy io_context_read", io_destroy(io_context_read));
	SYSCALL_ERR_HANDLE_PTHREAD("io_destroy io_context_write", io_destroy(io_context_write));
}



static void* sync_copy_thread_callback(void* copy_thread_params) {
    copy_thread_params_t* params = (copy_thread_params_t*) copy_thread_params;

	/* TODO: memory leak below! */
	uint8_t* copy_buffer = malloc(params->n_bytes);

    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;
    // read_args |= O_DIRECT; // disable kernel caching

    write_args = O_WRONLY;
    write_args |= O_CREAT;
    // write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE_PTHREAD("open (read)", (src_fd = open(params->input, read_args)));
    mode_t write_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    SYSCALL_ERR_HANDLE_PTHREAD("open (write)", (dest_fd = open(params->output, write_args, write_mode)));

	SYSCALL_ERR_HANDLE_PTHREAD("pread", pread(src_fd, copy_buffer, params->n_bytes, params->offset));

	SYSCALL_ERR_HANDLE_PTHREAD("pwrite", pwrite(dest_fd, copy_buffer, params->n_bytes, params->offset));

    return (void*)FCP_OK;
}



static void libaio_read_cb(io_context_t ctx, struct iocb *iocb, long res, long res2) {
	printf("libaio callback!\n");
	/*
	for (int num_conf = 0; num_conf < maxevents; num_conf++) {
		struct iocb* cur_iocb_read = read_configs + num_conf;

		if (cur_iocb_read != ev->obj) continue;

		if ((copy_states + num_conf)->was_read_finished) continue;

		printf("event %d res: %lu res2: %lu  bytes expected: %ld\n", num_conf, ev->res, ev->res2, (copy_states + num_conf)->n_bytes);

		SYSCALL_ERR_HANDLE_PTHREAD("ev->res (pread libaio)", ev->res);

		if (ev->res < (copy_states + num_conf)->n_bytes) continue;

		(copy_states + num_conf)->was_read_finished = true;
		printf("submitting event %d data: %p \n", num_conf, cur_iocb_read->data);
		read_events_done++;

		// TODO: check for ev->res (number of bytes written)
		SYSCALL_ERR_HANDLE_PTHREAD("io_submit (write event)", io_submit(io_context_write, 1, (write_configs_ptrs + num_conf)));

	}
	*/

	int num_conf = (int)(iocb->data);
	size_t should_write = iocb->u.c.nbytes;
	size_t wrote = res;

	int response_submit = io_submit(io_context_write, 1, write_configs_ptrs + num_conf);

	printf("callback nr %d, should write: %lu, wrote: %lu, io_submit response: %d\n", num_conf, should_write, wrote, response_submit);
}


static FCP_ERROR assert_file_type(struct stat* sb) {
    if ((sb->st_mode & S_IFMT) != S_IFREG) return FCP_BAD_FILE_TYPE;

    return FCP_OK;
}


static FCP_ERROR get_file_size(struct stat* sb, off_t* out) {
    *out = sb->st_size;

    return FCP_OK;
}
