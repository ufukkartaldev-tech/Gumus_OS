#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

void printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int sscanf(const char* str, const char* format, ...);

#endif
