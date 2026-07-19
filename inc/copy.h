#ifndef COPY_H
#define COPY_H

#include <stdint.h>
#include <stdbool.h>

#include "error_codes.h"

typedef struct {
    uint32_t threads;
    uint32_t queue_depth;
    const char* src;
    const char* dest;
    bool async;
	size_t fs_block_size;
} fcp_copy_config_t;

typedef struct {
    uint64_t elapsed_ns;
} fcp_copy_output_t;

FCP_ERROR fcp_copy(fcp_copy_config_t* config, fcp_copy_output_t* output);

#endif

