#ifndef GUMUS_LIB_H
#define GUMUS_LIB_H

#include <stdint.h>

// Syscall Numbers
#define SYS_EXIT  1
#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6
#define SYS_SBRK  7
#define SYS_KILL  8
#define SYS_SHM_GET 9
#define SYS_SHM_AT  10

// Wrapper Functions
static inline int syscall(int num, int a, int b, int c) {
    int ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(num), "b"(a), "c"(b), "d"(c));
    return ret;
}

static inline void exit(int code) {
    syscall(SYS_EXIT, code, 0, 0);
}

static inline int kill(int pid) {
    return syscall(SYS_KILL, pid, 0, 0);
}

static inline int shm_get(uint32_t key, uint32_t size) {
    return syscall(SYS_SHM_GET, key, size, 0);
}

static inline void* shm_at(int shm_id) {
    return (void*)syscall(SYS_SHM_AT, shm_id, 0, 0);
}

static inline int write(int fd, const void* buf, uint32_t size) {
    return syscall(SYS_WRITE, fd, (int)buf, size);
}

static inline int read(int fd, void* buf, uint32_t size) {
    return syscall(SYS_READ, fd, (int)buf, size);
}

static inline void print(const char* s) {
    int len = 0;
    while(s[len]) len++;
    write(1, s, len);
}

#endif
