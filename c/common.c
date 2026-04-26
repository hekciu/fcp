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
        printf("elapsed time -> %llu h %llu m %llu s %llu ms %llu us %llu ns\n", h, m, s, ms, us, ns);
    } else if (m > 0) {
        printf("elapsed time -> %llu m %llu s %llu ms %llu us %llu ns\n", m, s, ms, us, ns);
    } else if (s > 0) {
        printf("elapsed time -> %llu s %llu ms %llu us %llu ns\n", s, ms, us, ns);
    } else  if (ms > 0) {
        printf("elapsed time -> %llu ms %llu us %llu ns\n", ms, us, ns);
    } else if (us > 0) {
        printf("elapsed time -> %llu us %llu ns\n", us, ns);
    } else {
        printf("elapsed time -> %llu ns\n", ns);
    }
}
