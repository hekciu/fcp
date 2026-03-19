#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "config.h"


void fcp_exit(FCP_ERROR err) {
    fputs("Something went wrong try, 'fcp --help'\n", stderr);

    exit(err);
}

void fcp_print_help_message() {
    puts(CFG_HELP_MESSAGE);
}
