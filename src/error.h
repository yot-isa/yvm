#ifndef ERROR_H_
#define ERROR_H_

#include <stdarg.h>

void print_usage(void);
void exit_with_error(const char *format, ...);

#endif // ERROR_H_
