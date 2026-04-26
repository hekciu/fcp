#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "error_codes.h"
#include "common.h"
#include "copy.h"



bool compare_arg(int argc, char** argv, uint32_t index, const char* value) {
    if (argc <= index) return false;

    return strcmp(argv[index], value) == 0;
}


int main(int argc, char** argv) {
    if (compare_arg(argc, argv, 1, "--help")) {
        fcp_print_help_message();

        return 0;
    }

    if (argc < 3) fcp_exit(FCP_BAD_ARGUMENTS);

    fcp_copy_config_t config = {0};
    fcp_copy_output_t output = {0};

    config.src = argv[1];
    config.dest = argv[2];

    HANDLE_ERROR(fcp_copy(&config, &output));

    printf("elapsed: %llu nanoseconds\n", output.elapsed_ns);

    return 0;
}
