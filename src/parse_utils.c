#include <assert.h>

#include "parse_utils.h"

const char *shift(int *argc, const char ***argv)
{
    assert(*argc > 0);
    const char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

static bool parse_address(const char* string, size_t length, size_t *output, struct Address_Parse_Error *error)
{
    const char *beginning = string;
    char base = 10;

    if (length == 0) {
        *error = (struct Address_Parse_Error) {
            .kind = ADDRESS_PARSE_ERROR_KIND_ADDRESS_EXPECTED,
            .position = string
        };
        return true;
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

        switch (base) {
        case 2:
            if (c < '0' || c > '1') {
                *error = (struct Address_Parse_Error) {
                    .kind = ADDRESS_PARSE_ERROR_KIND_INVALID_CHARACTER,
                    .position = string
                };
                return true;
            }
            result *= 2;
            break;
        case 10:
            if (c < '0' || c > '9') {
                *error = (struct Address_Parse_Error) {
                    .kind = ADDRESS_PARSE_ERROR_KIND_INVALID_CHARACTER,
                    .position = string
                };
                return true;
            }
            result *= 10;
            break;
        case 16:
            if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) {
                *error = (struct Address_Parse_Error) {
                    .kind = ADDRESS_PARSE_ERROR_KIND_INVALID_CHARACTER,
                    .position = string
                };
                return true;
            }
            result <<= 4;
            break;
        }

        if (result < previous_result) {
            *error = (struct Address_Parse_Error) {
                .kind = ADDRESS_PARSE_ERROR_KIND_ADDRESS_TOO_BIG,
                .position = beginning
            };
            return true;
        } else {
            previous_result = result;
        }

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
            *error = (struct Address_Parse_Error) {
                .kind = ADDRESS_PARSE_ERROR_KIND_ADDRESS_TOO_BIG,
                .position = beginning
            };
            return true;
        } else {
            previous_result = result;
        }

        ++string;
        --length;
    }

    *output = result;
    return false;
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

static struct Address_Range_Parse_Error translate_address_to_address_range_parse_error(const struct Address_Parse_Error error)
{
    return (struct Address_Range_Parse_Error) {
        .kind = (enum Address_Range_Parse_Error_Kind) error.kind,
        .position = error.position
    };
}

bool parse_address_range(const char *string, struct Address_Range *output, struct Address_Range_Parse_Error *error)
{
    struct Address_Parse_Error address_parse_error;
 
    const char *address_position = string;
    const char *next_delimiter_position = get_next_delimiter_position(address_position);
    size_t address_length = (size_t) (next_delimiter_position - address_position);
    size_t address;

    bool has_errored = parse_address(address_position, address_length, &address, &address_parse_error);
    if (has_errored) {
        *error = translate_address_to_address_range_parse_error(address_parse_error);
        return true;
    }

    if (*next_delimiter_position == '-') {
        string = next_delimiter_position + 1;
        const char *address2_position = string;
        const char *next_end_of_string_position = get_next_end_of_string_position(address2_position);
        size_t address2_length = (size_t) (next_end_of_string_position - address2_position);
        size_t address2;

        has_errored = parse_address(address2_position, address2_length, &address2, &address_parse_error);
        if (has_errored) {
            *error = translate_address_to_address_range_parse_error(address_parse_error);
            return true;
        }

        if (address2 < address) {
            *error = (struct Address_Range_Parse_Error) {
                .kind = ADDRESS_RANGE_PARSE_ERROR_KIND_INVALID_ORDER,
                .position = address_position,
            };
            return true;
        }

        *output = (struct Address_Range) {
            .start_address = address,
            .end_offset = address2 - address
        };
    } else {
        *output = (struct Address_Range) {
            .start_address = address,
            .end_offset = 0
        };
    }

    return false;
}
