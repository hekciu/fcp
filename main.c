#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

#include "error_codes.h"
#include "common.h"
#include "copy.h"

static struct option long_opt[] =
{
  {"help", no_argument, NULL, 'h'},
  {"input", required_argument, NULL, 'i'},
  {"output", required_argument, NULL, 'o'},
  {"raw", required_argument, NULL, 'r'},
  {NULL, 0, NULL, 0}
};

static const char*  short_opt = "hior";

int main(int argc, char** argv) {
    fcp_copy_config_t config = {0};
    fcp_copy_output_t output = {0};

    char c;

    while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch(c)
        {
            case -1:       /* no more arguments */
            case 0:        /* long options toggles */
            break;

            case 'h':
            fcp_print_help_message();
            return 0;
            break;

            case 'i':
            config.src = argv[optind];
            break;

            case 'o':
            config.dest = argv[optind];
            break;

            case 'r':
            break;

            default:
            fcp_exit(FCP_BAD_ARGUMENTS);
        };
    }

    if (config.src == NULL || config.dest == NULL) fcp_exit(FCP_BAD_ARGUMENTS);

    HANDLE_ERROR(fcp_copy(&config, &output));

    fcp_print_time(output.elapsed_ns);

    return 0;
}
