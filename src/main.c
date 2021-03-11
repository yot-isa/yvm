#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "address_bus.h"
#include "cpu.h"
#include "error.h"
#include "parse_utils.h"
#include "devices/rom.h"

static int parse_yot_type(const char *string, enum Yot_Type *yot_type)
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

int main(int argc, const char **argv)
{
    shift(&argc, &argv);
    
    struct Address_Bus address_bus = {0};
    bool is_yot_type_set = false;
    enum Yot_Type yot_type = {0};

    int positional_index = 0;

    while (argc > 0) {
        const char *token = shift(&argc, &argv);

        if (strcmp(token, "-h") == 0 || strcmp(token, "--help") == 0) {
            print_usage();
            exit(0);
        } else if (strcmp(token, "--rom") == 0) {
            rom_parse_args(&address_bus, &argc, &argv);
        } else {
            switch (positional_index) {
            case 0:
                if (parse_yot_type(token, &yot_type)) {
                    exit_with_error("ERROR: Unknown Yot type `%s`.", token);
                } else {
                    is_yot_type_set = true;
                }
                break;
            default:
                if (token[0] == '-') {
                    exit_with_error("Unknown option `%s`.", token);
                } else {
                    exit_with_error("Unknown positional argument `%s`.", token);
                }
            }
            ++positional_index;
        }
    }

    if (!is_yot_type_set) {
        exit_with_error("No Yot type provided.");
    }

    struct Cpu cpu = (struct Cpu) {
        .type = yot_type,
        .ip = 0,
        .sp = 0,
        .halt = false,
        .carry = false
    };

    execute_next_instruction(&cpu, &address_bus);

    // struct Device device = address_bus.devices[0];

    exit(0);
}
