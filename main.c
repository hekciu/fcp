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


static struct option long_opt[] =
{
  {"help", no_argument, NULL, 'h'},
  {"input", required_argument, NULL, 'i'},
  {"output", required_argument, NULL, 'o'},
  {"threads", required_argument, NULL, 't'},
  {"queue_depth", required_argument, NULL, 'q'},
  {"fs_block_size", required_argument, NULL, 'b'},
  {"legacy", no_argument, NULL, 'l'},
  {"direct", no_argument, NULL, 'd'},
  {"csv", no_argument, NULL, 'c'},
  {NULL, 0, NULL, 0}
};

static const char*  short_opt = "hiotqbldc";

int main(int argc, char** argv) {
    fcp_copy_config_t config = {0};
    fcp_copy_output_t output = {0};

    /* defaults */
    bool output_csv_row = false;
    config.threads = 1;
    config.queue_depth = 0;
    config.async = false;
	config.fs_block_size = 4096;
	config.use_legacy_libaio = false;
	config.use_direct = false;

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

            case 'l':
            config.use_legacy_libaio = true;
            break;

            case 'd':
            config.use_direct = true;
            break;

            case 'c':
            output_csv_row = true;
            break;

            case 'q':
            HANDLE_ERROR(fcp_parse_ul(optarg, &config.queue_depth));
            config.async = true;
            break;

            case 'b':
            HANDLE_ERROR(fcp_parse_ul(optarg, &config.fs_block_size));
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
		fprintf(stderr, "file size must be divisible by block size: %d, got %ld\n", config.fs_block_size, input_stat.st_size);
		return FCP_BAD_FILE_SIZE;
	}

	if (((input_stat.st_size / config.threads) % config.fs_block_size) != 0) {
		fprintf(stderr, "(file size / threads) must be divisible by block size: %d, got %ld\n", config.fs_block_size, input_stat.st_size / config.threads);
		return FCP_BAD_FILE_SIZE;
	}

	if (config.async && (((input_stat.st_size / (config.queue_depth*config.threads)) % config.queue_depth) != 0)) {
		fprintf(stderr, "with async (file size / (queue_depth*threads) must be divisible by block size: %d, got %ld\n",
			config.fs_block_size,
			input_stat.st_size / (config.queue_depth*config.threads));

		return FCP_BAD_FILE_SIZE;
	}

	if ((config.fs_block_size % sizeof(void*)) != 0) {
		fprintf(stderr, "fs block size must be divisible by sizeof(void*): %ld, got %d\n", sizeof(void*), config.fs_block_size);
		return FCP_BAD_FILE_SIZE;
	}

	if ((config.fs_block_size) > 2147479552) {
		fprintf(stderr, "single write block (config.fs_block_size) in linux cannot be bigger than 2147479552\n");

		return FCP_BAD_FILE_SIZE;
	}

    HANDLE_ERROR(fcp_copy(&config, &output));

    double read_bytes_per_second =
        ((double)input_stat.st_size) / (((double)output.read_elapsed_ns) / 1000000000.0); 

    double read_megabytes_per_second = read_bytes_per_second / 1000000.0;

    double write_bytes_per_second =
        ((double)input_stat.st_size) / (((double)output.write_elapsed_ns) / 1000000000.0); 

    double write_megabytes_per_second = write_bytes_per_second / 1000000.0;

    if (true == output_csv_row) {
        char* io_type = NULL;

        if (false == config.async)
        {
            io_type = "psync";
        }
        else if (true == config.use_legacy_libaio)
        {
            io_type = "libaio";
        }
        else
        {
            io_type = "io_uring";
        }

        /* num_threads:type[psync/libaio/io_uring]:direct(true/false):queue_depth:file_size:fs_block_size:read_megabytes_per_second:write_megabytes_per_second */
        printf("%u,%s,%i,%u,%lu,%u,%lf,%lf\n",
            config.threads,
            io_type,
            config.use_direct,
            config.queue_depth,
            input_stat.st_size,
            config.fs_block_size,
            read_megabytes_per_second,
            write_megabytes_per_second);

        return 0;
    }

	printf("input: '%s', output: '%s'\n", config.src, config.dest);
    printf("number of threads: %u, queue depth: %u\n", config.threads, config.queue_depth);

    printf("read: ");
    fcp_print_time(output.read_elapsed_ns);
    printf("write: ");
    fcp_print_time(output.write_elapsed_ns);

    printf("read megabytes per second: %f\n", read_megabytes_per_second);

    printf("write megabytes per second: %f\n", write_megabytes_per_second);

    return 0;
}
