#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "error_codes.h"

void fcp_exit(FCP_ERROR err);
void fcp_print_help_message();

#define SYSCALL_ERR_HANDLE(val) {if(val < 0) {fprintf(stderr, "%s\n", strerrordesc_np(errno));return FCP_SYSCALL_FAILED;}}

#endif
