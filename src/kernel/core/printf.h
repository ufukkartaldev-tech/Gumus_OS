#ifndef PRINTF_H
#define PRINTF_H

#include <stdarg.h>

void printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int vsprintf(char* str, const char* format, va_list args);
int sscanf(const char* str, const char* format, ...);

#endif
