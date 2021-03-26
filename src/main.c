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
        } else if (strcmp(token, "--ram") == 0) {
            ram_parse_args(&address_bus, &argc, &argv);
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

    struct Cpu cpu;
    cpu_initialize(&cpu, &address_bus, yot_type);

    size_t ROM_SIZE = get_address_range_length(&address_bus.address_range_table[0].address_range);
    size_t INITIAL_DSP = cpu.dsp;
    size_t INITIAL_ASP = cpu.asp;
    size_t instruction_count = 0;
    while (!cpu.break_flag && instruction_count < 100) {
        execute_next_instruction(&cpu, &address_bus);
        instruction_count += 1;

        printf("  ");
        for (size_t b = 0; b < cpu.dsp - INITIAL_DSP; ++b) {
            printf("%02X ", address_bus.devices[1].contents.as_ram->data[b + INITIAL_DSP - ROM_SIZE]);
        }
        printf("; ");
        for (size_t b = 0; b < cpu.asp - INITIAL_ASP; ++b) {
            printf("%02X ", address_bus.devices[1].contents.as_ram->data[b + INITIAL_ASP - ROM_SIZE]);
        }
        printf("\n");
    }

    // struct Device device = address_bus.devices[0];

    exit(0);
}
