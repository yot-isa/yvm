#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address_bus.h"
#include "parse_utils.h"
#include "devices/rom.h"

static void print_usage(FILE *stream, const char *program_name)
{
    fprintf(
        stream,
        "Usage:\n"
        "    %s [FLAGS] <YOT TYPE>\n"
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

typedef enum {
    YOT_8,
    YOT_16,
    YOT_32,
    YOT_64
} Yot_Type;

static int parse_yot_type(const char *string, Yot_Type *yot_type)
{
    if (strcmp(string, "yot-8") == 0) {
        *yot_type = YOT_8;
    } else if (strcmp(string, "yot-16") == 0) {
        *yot_type = YOT_16;
    } else if (strcmp(string, "yot-32") == 0) {
        *yot_type = YOT_32;
    } else if (strcmp(string, "yot-64") == 0) {
        *yot_type = YOT_64;
    } else {
        return -1;
    }

    return 0;
}

static void print_unknown_yot_type(FILE *stream, const char *string, const char *program_name)
{
    fprintf(stream, "ERROR: Unknown Yot type `%s`. "
            "Possible values are: `yot-8`, `yot-16`, `yot-32`, and `yot-64`.\n", string);
    print_usage(stream, program_name);

}

int main(int argc, const char **argv)
{
    const char *program_name = shift(&argc, &argv);
    
    struct Address_Bus address_bus = {0};
    bool is_yot_type_set = false;
    Yot_Type yot_type;

    int positional_index = 0;

    while (argc > 0) {
        const char *token = shift(&argc, &argv);

        if (strcmp(token, "-h") == 0 || strcmp(token, "--help") == 0) {
            print_usage(stderr, program_name);
            exit(0);
        } else if (strcmp(token, "--rom") == 0) {
            rom_parse_args(&address_bus, &argc, &argv);
        } else {
            switch (positional_index) {
            case 0:
                if (parse_yot_type(token, &yot_type)) {
                    print_unknown_yot_type(stderr, token, program_name);
                    exit(1);
                } else {
                    is_yot_type_set = true;
                }
                break;
            default:
                print_unknown_argument(stderr, token, program_name);
                exit(1);
            }
            ++positional_index;
        }
    }

    if (!is_yot_type_set) {
        fprintf(stderr, "ERROR: No Yot type provided.\n");
        print_usage(stderr, program_name);
        exit(1);
    }

    // struct Device device = address_bus.devices[0];

    exit(0);
}
