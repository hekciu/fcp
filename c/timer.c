#include <stdint.h>

#include "timer.h"


FCP_ERROR start_timer(fcp_timer_t* timer) {
    SYSCALL_ERR_HANDLE("clock_gettime", clock_gettime(CFG_TIMER_CLOCK, &timer->_start));

    return FCP_OK;
}


FCP_ERROR stop_timer(fcp_timer_t* timer) {
    SYSCALL_ERR_HANDLE("clock_gettime", clock_gettime(CFG_TIMER_CLOCK, &timer->_end));

    timer->elapsed_ns = 1e9 * timer->_end.tv_sec + timer->_end.tv_nsec
        - (1e9 * timer->_start.tv_sec + timer->_start.tv_nsec);

    return FCP_OK;
}
