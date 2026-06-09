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

void fcp_print_time(uint64_t elapsed_ns) {
    uint64_t h = elapsed_ns / (60 * 60 * 1e9);
    elapsed_ns -= h * (60 * 60 * 1e9);

    uint64_t m = elapsed_ns / (60 * 1e9);
    elapsed_ns -= m * (60 * 1e9);

    uint64_t s = elapsed_ns / 1e9;
    elapsed_ns -= s * 1e9;

    uint64_t ms = elapsed_ns / 1e6;
    elapsed_ns -= ms * 1e6;

    uint64_t us = elapsed_ns / 1e3;
    elapsed_ns -= us * 1e3;

    uint64_t ns = elapsed_ns;

    if (h > 0) {
        printf("elapsed time -> %lu h %lu m %lu s %lu ms %lu us %lu ns\n", h, m, s, ms, us, ns);
    } else if (m > 0) {
        printf("elapsed time -> %lu m %lu s %lu ms %lu us %lu ns\n", m, s, ms, us, ns);
    } else if (s > 0) {
        printf("elapsed time -> %lu s %lu ms %lu us %lu ns\n", s, ms, us, ns);
    } else  if (ms > 0) {
        printf("elapsed time -> %lu ms %lu us %lu ns\n", ms, us, ns);
    } else if (us > 0) {
        printf("elapsed time -> %lu us %lu ns\n", us, ns);
    } else {
        printf("elapsed time -> %lu ns\n", ns);
    }
}

FCP_ERROR fcp_parse_ul(const char* input, uint32_t* output) {
    errno = 0;

    *output = strtoul(input, NULL, 10);

    if (errno != 0) return FCP_UINT32_PARSE_FAILED;

    return FCP_OK;
}
