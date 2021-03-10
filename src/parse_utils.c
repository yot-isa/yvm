#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse_utils.h"

const char *shift(int *argc, const char ***argv)
{
    assert(*argc > 0);
    const char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static size_t parse_address(const char* string, size_t length)
{
    const char *beginning = string;
    char base = 10;

    if (length == 0) {
        fprintf(stderr, "ERROR: address expected in address range\n");
        exit(1);
    }

    if (length > 2 && string[0] == '0' && (string[1] == 'b' || string[1] == 'x')) {
        switch (string[1]) {
        case 'b':
            base = 2;
            break;
        case 'x':
            base = 16;
            break;
        }
        string += 2;
        length -= 2;
    }

    size_t result = 0, previous_result = 0;

    while (length > 0) {
        char c = *string;

        if ((base == 2 && (c < '0' || c > '1')) ||
                (base == 10 && (c < '0' || c > '9')) ||
                (base == 16 && ((c < '0' || c > '9') && (c < 'a' || c > 'f')))) {
            fprintf(stderr, "ERROR: invalid character `%c` in address range\n", c);
            exit(1);
        }

        result *= (size_t) base;
        
        if (result < previous_result) {
            fprintf(stderr, "ERROR: address `%.*s` in address range is too big\n", (int) length, beginning);
            exit(1);
        }

        previous_result = result;

        switch (base) {
        case 2:
            result |= (size_t) (c - '0');
            break;
        case 10:
            result += (size_t) (c - '0');
            break;
        case 16:
            if (c >= '0' && c <= '9') {
                result |= (size_t) (c - '0');
            } else {
                result |= (size_t) (c - 'a' + 10);
            }
            break;
        }

        if (result < previous_result) {
            fprintf(stderr, "ERROR: address `%.*s` in address range is too big\n", (int) length, beginning);
            exit(1);
        }

        previous_result = result;

        ++string;
        --length;
    }

    return result;
}

static const char *get_next_delimiter_position(const char *string)
{
    while (*string != '-' && *string != '\0') {
        ++string;
    }
    return string;
}

static const char *get_next_end_of_string_position(const char *string)
{
    while (*string != '\0') {
        ++string;
    }
    return string;
}

struct Address_Range parse_address_range(const char *string)
{
    const char *beginning = string;

    const char *address_position = string;
    const char *next_delimiter_position = get_next_delimiter_position(address_position);
    size_t address_length = (size_t) (next_delimiter_position - address_position);

    size_t address = parse_address(address_position, address_length);

    if (*next_delimiter_position == '-') {
        string = next_delimiter_position + 1;
        const char *address2_position = string;
        const char *next_end_of_string_position = get_next_end_of_string_position(address2_position);
        size_t address2_length = (size_t) (next_end_of_string_position - address2_position);

        size_t address2 = parse_address(address2_position, address2_length);

        if (address2 < address) {
            fprintf(stderr, "ERROR: invalid address order in address range `%s`\n", beginning);
            exit(1);
        }

        return (struct Address_Range) {
            .start_address = address,
            .end_offset = address2 - address
        };
    } else {
        return (struct Address_Range) {
            .start_address = address,
            .end_offset = 0
        };
    }
}
