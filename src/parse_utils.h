#ifndef PARSE_UTILS_H_
#define PARSE_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

#include "address_bus.h" 

const char *shift(int *argc, const char ***argv);

struct Address_Range parse_address_range(const char *string);

#endif // PARSE_UTILS_H_
