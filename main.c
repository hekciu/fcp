#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "error_codes.h"
#include "common.h"
#include "copy.h"


/* File-system related */
#define FS_BLOCK_SIZE 4096

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
	config.fs_block_size = FS_BLOCK_SIZE;

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
            config.src = optarg;
            break;

            case 'o':
            config.dest = optarg;
            break;

            case 'r':
            output_raw = true;
            break;

            case 'q':
            HANDLE_ERROR(fcp_parse_ul(optarg, &config.queue_depth));
            config.async = true;
            break;

            case 't':
            HANDLE_ERROR(fcp_parse_ul(optarg, &config.threads));
            break;

            default:
            fcp_exit(FCP_BAD_ARGUMENTS);
        };
    }

    if (config.src == NULL || config.dest == NULL) fcp_exit(FCP_BAD_ARGUMENTS);

	struct stat input_stat = {0};
	SYSCALL_ERR_HANDLE("stat", stat(config.src, &input_stat));

	if ((input_stat.st_size % config.fs_block_size) != 0) {
		fprintf(stderr, "file size must be divisible by block size: %ld, got %ld\n", config.fs_block_size, input_stat.st_size);
		return FCP_BAD_FILE_SIZE;
	}

	if (((input_stat.st_size / config.threads) % config.fs_block_size) != 0) {
		fprintf(stderr, "(file size / threads) must be divisible by block size: %ld, got %ld\n", config.fs_block_size, input_stat.st_size / config.threads);
		return FCP_BAD_FILE_SIZE;
	}

	if (config.async && (((input_stat.st_size / (config.queue_depth*config.threads)) % config.queue_depth) != 0)) {
		fprintf(stderr, "with async (file size / (queue_depth*threads) must be divisible by block size: %ld, got %ld\n",
			config.fs_block_size,
			input_stat.st_size / (config.queue_depth*config.threads));
		return FCP_BAD_FILE_SIZE;
	}

	if ((config.fs_block_size % sizeof(void*)) != 0) {
		fprintf(stderr, "fs block size must be divisible by sizeof(void*): %ld, got %ld\n", sizeof(void*), config.fs_block_size);
		return FCP_BAD_FILE_SIZE;
	}

    HANDLE_ERROR(fcp_copy(&config, &output));

    if (output_raw) {
        printf("%lu", output.elapsed_ns);

        return 0;
    }

	printf("input: '%s', output: '%s'\n", config.src, config.dest);
    printf("number of threads: %u, queue depth: %u\n", config.threads, config.queue_depth);

    fcp_print_time(output.elapsed_ns);

    return 0;
}
