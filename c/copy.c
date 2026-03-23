#include <fcntl.h>
#include <sys/stat.h>

#include "copy.h"
#include "common.h"

static FCP_ERROR assert_file_type(int fd);
static FCP_ERROR get_file_size(int fd, off_t* out);


FCP_ERROR fcp_copy(copy_config_t* config) {
    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;

    read_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE((src_fd = open(config->src, read_args)));

    write_args = O_WRONLY;

    write_args = O_CREAT; // create new file
    write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE((dest_fd = open(config->dest, write_args)));

    HANDLE_ERROR(assert_file_type(src_fd));

    return FCP_OK;
}


static FCP_ERROR assert_file_type(int fd) {
    struct stat statbuf = {0};

    SYSCALL_ERR_HANDLE(fstat(fd, &statbuf));

    if ((statbuf.st_mode & S_IFMT) != S_IFREG) return FCP_BAD_FILE_TYPE;

    return FCP_OK;
}


static FCP_ERROR get_file_size(int fd, off_t* out) {
    struct stat statbuf = {0};

    SYSCALL_ERR_HANDLE(fstat(fd, &statbuf));

    *out = statbuf.st_size;

    return FCP_OK;
}
