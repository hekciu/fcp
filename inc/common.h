#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error_codes.h"

void fcp_exit(FCP_ERROR err);
void fcp_print_help_message();

#define SYSCALL_ERR_HANDLE(name, val) if(val < 0) {fprintf(stderr, "%s failed, details: \n%s\n", name, strerrordesc_np(errno));return FCP_SYSCALL_FAILED;}

#define HANDLE_ERROR(fcp_err) {if ((fcp_err) != FCP_OK) { fcp_exit(fcp_err); }}

#endif
