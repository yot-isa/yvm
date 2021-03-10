#include <stdio.h>
#include <stdlib.h>

#include "error.h"

#define RED   "\x1B[31m"
#define RESET "\x1B[0m"

void print_usage(void)
{
    fprintf(
        stderr,
        "Usage:\n"
        "    yvm [FLAGS] <YOT TYPE>\n"
        "\n"
        "FLAGS:\n"
        "    -h, --help\n"
        "        Prints help information.\n"
    );
}

void exit_with_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stderr, RED "error: " RESET);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);

    print_usage();

    exit(1);
}
