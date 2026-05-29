#ifndef CONFIG_H
#define CONFIG_H

#define CFG_HELP_MESSAGE "FCP - fast cp\n" \
    "    @param -h --help -> print this message\n" \
    "    @param -i --input -> input file\n" \
    "    @param -o --output -> output file\n" \
    "    @param -r --raw -> output raw copy time nanoseconds\n" \
    "    @param -q --queue_depth -> use async I/O, specify queue depth\n" \
    "    @param -t --threads -> number of threads\n" \
    "   "

#define CFG_TIMER_CLOCK CLOCK_PROCESS_CPUTIME_ID

#endif

