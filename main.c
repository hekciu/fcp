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
  {"raw", no_argument, NULL, 'r'},
  {"threads", required_argument, NULL, 't'},
  {"queue_depth", required_argument, NULL, 'q'},
  {NULL, 0, NULL, 0}
};

static const char*  short_opt = "hiortq";

int main(int argc, char** argv) {
    fcp_copy_config_t config = {0};
    fcp_copy_output_t output = {0};

    /* defaults */
    bool output_raw = false;
    config.threads = 1;
    config.queue_depth = 0;
    config.async = false;

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
            output_raw = true;
            break;

            case 'q':
            HANDLE_ERROR(fcp_parse_ul(argv[optind], &config.queue_depth));
            config.async = true;
            break;

            case 't':
            HANDLE_ERROR(fcp_parse_ul(argv[optind], &config.threads));
            break;

            default:
            fcp_exit(FCP_BAD_ARGUMENTS);
        };
    }

    if (config.src == NULL || config.dest == NULL) fcp_exit(FCP_BAD_ARGUMENTS);

    HANDLE_ERROR(fcp_copy(&config, &output));

    if (output_raw) {
        printf("%llu", output.elapsed_ns);

        return 0;
    }

    printf("number of threads: %lu, queue depth: %lu\n", config.threads, config.queue_depth);

    fcp_print_time(output.elapsed_ns);

    return 0;
}
