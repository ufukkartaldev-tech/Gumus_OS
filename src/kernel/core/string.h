#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
size_t strlen(const char* s);
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
void itoa(int n, char* s);
void reverse(char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strtok(char* s, const char* delim);
int atoi(const char* s);

#endif
