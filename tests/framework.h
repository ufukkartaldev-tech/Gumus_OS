#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>

/**
 * GümüşOS Test Framework (Pro Seviye)
 * ----------------------------------
 * Detaylı hata raporlama ve OS seviyesi kontroller içerir.
 */

#define TEST_PASS 1
#define TEST_FAIL 0

static int _cy = 4;

static void _print_raw(const char* s, int x, int y, uint8_t c) {
    uint16_t* v = (uint16_t*)0xB8000;
    for (int i = 0; s[i]; i++) v[y * 80 + x + i] = (uint16_t)s[i] | ((uint16_t)c << 8);
}

static void test_header(const char* t) {
    uint16_t* v = (uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) v[i] = (uint16_t)' ' | (0x0F << 8);
    _print_raw("--- GUMUS OS RIGOROUS TEST SUITE ---", 22, 1, 0x0B);
    _print_raw(t, 2, 2, 0x0E);
    _cy = 4;
}

// Detaylı itoa (framework içinde bağımsız olması için)
static void _test_itoa(uint32_t n, char* s, int base) {
    char* p = s;
    char* q = s;
    uint32_t tmp;
    if (n == 0) *p++ = '0';
    while (n > 0) {
        tmp = n % base;
        *p++ = (tmp < 10) ? (tmp + '0') : (tmp - 10 + 'A');
        n /= base;
    }
    *p-- = '\0';
    while (q < p) {
        char t = *q;
        *q++ = *p;
        *p-- = t;
    }
}

static void ASSERT_DETAIL(int cond, const char* msg, uint32_t expected, uint32_t actual) {
    if (!cond) {
        _print_raw("[FAIL]", 2, _cy, 0x0C);
        _print_raw(msg, 9, _cy, 0x0F);
        
        char buf[32];
        _print_raw(" (Exp:", 40, _cy, 0x07);
        _test_itoa(expected, buf, 16); _print_raw(buf, 46, _cy, 0x0E);
        _print_raw(" Got:", 55, _cy, 0x07);
        _test_itoa(actual, buf, 16); _print_raw(buf, 60, _cy, 0x0E);
        _print_raw(")", 75, _cy, 0x07);
        
        _cy++;
    }
}

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            _print_raw("[FAIL]", 2, _cy, 0x0C); \
            _print_raw(msg, 9, _cy, 0x0F); \
            _cy++; return TEST_FAIL; \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            ASSERT_DETAIL(0, msg, (uint32_t)(expected), (uint32_t)(actual)); \
            return TEST_FAIL; \
        } \
    } while (0)

#define RUN_TEST(func, name) \
    do { \
        if (_cy > 23) { test_header(name); } \
        _print_raw("Running: ", 2, _cy, 0x07); \
        _print_raw(name, 11, _cy, 0x0F); \
        if (func()) { \
            _print_raw("[PASS]", 72, _cy, 0x0A); \
        } else { \
            _print_raw("[FAIL]", 72, _cy, 0x0C); \
        } \
        _cy++; \
    } while (0)

#endif
