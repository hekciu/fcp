#ifndef COPY_H
#define COPY_H

#include <stdint.h>

#include "error_codes.h"

typedef struct {
    uint32_t n_threads;
    uint32_t queue_depth;
} copy_config_t;

FCP_ERROR copy(char* src, char* dir, copy_config_t* config);

#endif

