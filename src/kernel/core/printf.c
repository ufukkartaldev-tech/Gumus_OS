#include "printf.h"
#include "kernel.h"
#include "string.h"

int vsprintf(char* str, const char* format, va_list args) {
    char* start = str;
    while (*format) {
        if (*format == '%') {
            format++;
            if (*format == 's') {
                char* s = va_arg(args, char*);
                while (*s) *str++ = *s++;
            } else if (*format == 'd') {
                int d = va_arg(args, int);
                char buf[32];
                itoa(d, buf);
                char* p = buf;
                while (*p) *str++ = *p++;
            } else if (*format == 'x' || *format == 'X') {
                uint32_t x = va_arg(args, uint32_t);
                char buf[32];
                const char* hex = "0123456789ABCDEF";
                int i = 0;
                if (x == 0) buf[i++] = '0';
                else {
                    while (x > 0) {
                        buf[i++] = hex[x % 16];
                        x /= 16;
                    }
                }
                buf[i] = '\0';
                reverse(buf);
                char* p = buf;
                while (*p) *str++ = *p++;
            } else if (*format == 'c') {
                *str++ = (char)va_arg(args, int);
            } else if (*format == '%') {
                *str++ = '%';
            }
        } else {
            *str++ = *format;
        }
        format++;
    }
    *str = '\0';
    return str - start;
}

int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int res = vsprintf(str, format, args);
    va_end(args);
    return res;
}

void printf(const char* format, ...) {
    char buf[1024]; // Temporary buffer for simplicity
    va_list args;
    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);
    print(buf);
}

int sscanf(const char* str, const char* format, ...) {
    // VERY limited sscanf. Only handles %d.%d.%d.%d for IP parsing
    if (strcmp(format, "%d.%d.%d.%d") != 0) return 0;
    
    va_list args;
    va_start(args, format);
    
    int* a = va_arg(args, int*);
    int* b = va_arg(args, int*);
    int* c = va_arg(args, int*);
    int* d = va_arg(args, int*);
    
    const char* p = str;
    *a = atoi(p);
    while (*p && *p != '.') p++;
    if (*p) p++;
    *b = atoi(p);
    while (*p && *p != '.') p++;
    if (*p) p++;
    *c = atoi(p);
    while (*p && *p != '.') p++;
    if (*p) p++;
    *d = atoi(p);
    
    va_end(args);
    return 4;
}
