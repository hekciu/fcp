#ifndef COPY_H
#define COPY_H

#include <stdint.h>

#include "error_codes.h"

typedef struct {
    uint32_t n_threads;
    uint32_t queue_depth;
    const char* src;
    const char* dest;
} copy_config_t;

FCP_ERROR fcp_copy(copy_config_t* config);

#endif

