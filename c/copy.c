#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "copy.h"
#include "common.h"

static FCP_ERROR assert_file_type(struct stat* sb);
static FCP_ERROR get_file_size(struct stat* sb, off_t* out);

#define COPY_BUFFER_SIZE 2000
static char copy_buffer[COPY_BUFFER_SIZE];


FCP_ERROR fcp_copy(copy_config_t* config) {
    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;
    // read_args |= O_DIRECT; // disable kernel caching

    SYSCALL_ERR_HANDLE("open (read)", (src_fd = open(config->src, read_args)));

    write_args = O_WRONLY;
    write_args |= O_CREAT;
    // write_args |= O_DIRECT | O_SYNC; // disable kernel caching
    mode_t write_mode = S_IRWXU | S_IRWXG | S_IRWXO;

    SYSCALL_ERR_HANDLE("open (write)", (dest_fd = open(config->dest, write_args, write_mode)));

    struct stat src_sb = {0};
    SYSCALL_ERR_HANDLE("fstat", fstat(src_fd, &src_sb));

    HANDLE_ERROR(assert_file_type(&src_sb));

    off_t src_size = 0;

    HANDLE_ERROR(get_file_size(&src_sb, &src_size));

    size_t n_rounds = (src_size / COPY_BUFFER_SIZE) + 1;

    for (int i = 0; i < n_rounds; i++) {
        size_t bytes_left = src_size - (i * COPY_BUFFER_SIZE);

        size_t copy_bytes = bytes_left > COPY_BUFFER_SIZE ? COPY_BUFFER_SIZE : bytes_left;

        size_t offset = i * COPY_BUFFER_SIZE;

        SYSCALL_ERR_HANDLE("pread", pread(src_fd, copy_buffer, copy_bytes, offset));

        SYSCALL_ERR_HANDLE("pwrite", pwrite(dest_fd, copy_buffer, copy_bytes, offset));
    }

    return FCP_OK;
}


static FCP_ERROR assert_file_type(struct stat* sb) {
    if ((sb->st_mode & S_IFMT) != S_IFREG) return FCP_BAD_FILE_TYPE;

    return FCP_OK;
}


static FCP_ERROR get_file_size(struct stat* sb, off_t* out) {
    *out = sb->st_size;

    return FCP_OK;
}
