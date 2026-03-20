#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "error_codes.h"
#include "common.h"
#include "copy.h"


#define HANDLE_ERROR(int_val) {if ((int_val) != 0) { fcp_exit(int_val); }}

bool compare_arg(int argc, char** argv, uint32_t index, const char* value) {
    if (argc <= index) return false;

    return strcmp(argv[index], value) == 0;
}


int main(int argc, char** argv) {
    if (compare_arg(argc, argv, 1, "--help")) {
        fcp_print_help_message();

        return 0;
    }

    copy_config_t config = {0};

    copy("test", "test", &config);

    fcp_exit(FCP_BAD_ARGUMENTS);

    return 0;
}
