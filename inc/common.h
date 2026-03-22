#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <string.h>

#include "error_codes.h"

void fcp_exit(FCP_ERROR err);
void fcp_print_help_message();

#define SYSCALL_ERR_HANDLE(val) {if(val != 0) {fputs(strerrordesc_np(val), stderr);return FCP_SYSCALL_FAILED;}}

#endif
