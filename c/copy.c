#include <fcntl.h>

#include "copy.h"
#include "common.h"


FCP_ERROR fcp_copy(copy_config_t* config) {
    int src_fd, dest_fd, read_args, write_args;

    read_args = O_RDONLY;

    read_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE((src_fd = open(config->src, read_args)));

    write_args = O_WRONLY;

    write_args = O_CREAT; // create new file
    write_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE((dest_fd = open(config->dest, write_args)));

    return FCP_OK;
}
