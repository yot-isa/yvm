#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
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
        exit_with_error("Address expected in address range.");
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
            exit_with_error("Invalid character `%c` in address range.", c);
        }

        result *= (size_t) base;
        
        if (result < previous_result) {
            exit_with_error("Address `%.*s` in address range is too big.", (int) length, beginning);
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
            exit_with_error("Address `%.*s` in address range is too big.", (int) length, beginning);
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

struct Address_Range parse_address_range(const struct Address_Bus *address_bus, const char *string)
{
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
            exit_with_error("Invalid address order in the address range.");
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
