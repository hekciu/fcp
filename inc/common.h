#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "error_codes.h"

void fcp_exit(FCP_ERROR err);
void fcp_print_help_message();
void fcp_print_time(uint64_t elapsed_ns);

#define SYSCALL_ERR_HANDLE(name, call) do { if((call) < 0) {fprintf(stderr, "%s failed, details: \n%s\n", name, strerrordesc_np(errno));return FCP_SYSCALL_FAILED;} } while(0)

#define HANDLE_ERROR(call) \
{ \
    do \
    { \
        FCP_ERROR res = (call); \
        if (res != FCP_OK) \
        { \
            fcp_exit(res);\
        } \
    } \
    while(0); \
}


#endif
