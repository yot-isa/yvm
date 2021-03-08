#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *shift(int *argc, const char ***argv)
{
    assert(*argc > 0);
    const char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static void print_usage(FILE *stream, const char *program_name)
{
    fprintf(
        stream,
        "Usage:\n"
        "    %s [FLAGS]\n"
        "\n"
        "FLAGS:\n"
        "    -h, --help\n"
        "        Prints help information.\n",
        program_name
    );
}

static void print_unknown_argument(FILE *stream, const char *argument, const char *program_name)
{ 
    if (argument[0] == '-') {
        fprintf(stream, "ERROR: unknown option `%s`.\n", argument);
    } else {
        fprintf(stream, "ERROR: unknown positional argument `%s`.\n", argument);
    }
    print_usage(stream, program_name);
}

int main(int argc, const char **argv)
{
    const char *program_name = shift(&argc, &argv);

    int positional_index = 0;

    while (argc > 0) {
        const char *token = shift(&argc, &argv);

        if (strcmp(token, "-h") == 0 || strcmp(token, "--help") == 0) {
            print_usage(stderr, program_name);
            exit(0);
        } else {
            switch (positional_index) {
            default:
                print_unknown_argument(stderr, token, program_name);
                exit(1);
            }
            ++positional_index;
        }
    }

    print_usage(stderr, program_name);
    exit(0);
}
