#include <fcntl.h>

#include "copy.h"
#include "common.h"


FCP_ERROR fcp_copy(copy_config_t* config) {
    int open_args = O_RDONLY;

    open_args |= O_DIRECT | O_SYNC; // disable kernel caching

    SYSCALL_ERR_HANDLE(open(config->src, open_args));
}
