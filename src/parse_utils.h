#ifndef PARSE_UTILS_H_
#define PARSE_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

#include "address_bus.h" 

const char *shift(int *argc, const char ***argv);

enum Address_Parse_Error_Kind {
    ADDRESS_PARSE_ERROR_KIND_INVALID_CHARACTER,
    ADDRESS_PARSE_ERROR_KIND_ADDRESS_TOO_BIG,
    ADDRESS_PARSE_ERROR_KIND_ADDRESS_EXPECTED,
};

struct Address_Parse_Error {
    enum Address_Parse_Error_Kind kind;
    const char *position;
};

enum Address_Range_Parse_Error_Kind {
    ADDRESS_RANGE_PARSE_ERROR_KIND_INVALID_CHARACTER,
    ADDRESS_RANGE_PARSE_ERROR_KIND_ADDRESS_TOO_BIG,
    ADDRESS_RANGE_PARSE_ERROR_KIND_ADDRESS_EXPECTED,
    ADDRESS_RANGE_PARSE_ERROR_KIND_INVALID_ORDER,
};

struct Address_Range_Parse_Error {
    enum Address_Range_Parse_Error_Kind kind;
    const char *position;
};

bool parse_address_range(const char *string, struct Address_Range *output, struct Address_Range_Parse_Error *error);

#endif // PARSE_UTILS_H_
