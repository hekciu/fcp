#ifndef TIMER_H
#define TIMER_H

#include <time.h>

#include "config.h"
#include "common.h"

typedef struct {
    uint64_t elapsed_ns;

    struct timespec _start;
    struct timespec _end;
} fcp_timer_t;

FCP_ERROR start_timer(fcp_timer_t*);
FCP_ERROR stop_timer(fcp_timer_t*);

#endif
