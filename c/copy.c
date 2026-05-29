#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

#include "copy.h"
#include "common.h"
#include "timer.h"

typedef struct {
    const char* input;
    const char* output;
    size_t offset;
    off_t n_bytes;
} copy_thread_params_t;

static FCP_ERROR assert_file_type(struct stat* sb);
static FCP_ERROR get_file_size(struct stat* sb, off_t* out);

#define COPY_BUFFER_SIZE 2000

static void* copy_thread_callback(void* copy_thread_params);


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

    /* timer start */
    fcp_timer_t timer;
    start_timer(&timer);

    copy_thread_params_t params = {0};

    params.input = config->src;
    params.output = config->dest;
    params.offset = 0;
    params.n_bytes = src_size;

    pthread_t pthread_1;

    SYSCALL_ERR_HANDLE("pthread_create", pthread_create(&pthread_1,
				   NULL, 
				   copy_thread_callback,
				   (void*)&params));

    pthread_join(pthread_1, NULL);

    stop_timer(&timer);
    output->elapsed_ns = timer.elapsed_ns;

    return FCP_OK;
}



static void* copy_thread_callback(void* copy_thread_params) {
    char copy_buffer[COPY_BUFFER_SIZE];

    copy_thread_params_t* params = (copy_thread_params_t*) copy_thread_params;

    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;
    // read_args |= O_DIRECT; // disable kernel caching

    write_args = O_WRONLY;
    write_args |= O_CREAT;
    // write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE_PTHREAD("open (read)", (src_fd = open(params->input, read_args)));
    mode_t write_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    SYSCALL_ERR_HANDLE_PTHREAD("open (write)", (dest_fd = open(params->output, write_args, write_mode)));

    struct stat src_sb = {0};
    SYSCALL_ERR_HANDLE_PTHREAD("fstat", fstat(src_fd, &src_sb));

    size_t n_rounds = (params->n_bytes / COPY_BUFFER_SIZE) + 1;

    for (int i = 0; i < n_rounds; i++) {
        size_t bytes_left = params->n_bytes - (i * COPY_BUFFER_SIZE);

        size_t copy_bytes = bytes_left > COPY_BUFFER_SIZE ? COPY_BUFFER_SIZE : bytes_left;

        size_t offset = i * COPY_BUFFER_SIZE;

        SYSCALL_ERR_HANDLE_PTHREAD("pread", pread(src_fd, copy_buffer, copy_bytes, offset));

        SYSCALL_ERR_HANDLE_PTHREAD("pwrite", pwrite(dest_fd, copy_buffer, copy_bytes, offset));
    }

    return (void*)FCP_OK;
}


static FCP_ERROR assert_file_type(struct stat* sb) {
    if ((sb->st_mode & S_IFMT) != S_IFREG) return FCP_BAD_FILE_TYPE;

    return FCP_OK;
}


static FCP_ERROR get_file_size(struct stat* sb, off_t* out) {
    *out = sb->st_size;

    return FCP_OK;
}
