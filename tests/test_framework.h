#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>

// VGA Addresses and Constants
#define TEST_VGA_ADDR 0xB8000
#define TEST_VGA_WIDTH 80
#define TEST_VGA_HEIGHT 25

static int test_current_row = 1;

static void test_vga_putchar(char c, int x, int y, char color) {
    char* vga = (char*)TEST_VGA_ADDR;
    vga[y * TEST_VGA_WIDTH * 2 + x * 2] = c;
    vga[y * TEST_VGA_WIDTH * 2 + x * 2 + 1] = color;
}

static void test_vga_print(const char* str, int x, int y, char color) {
    for (int i = 0; str[i]; i++) {
        test_vga_putchar(str[i], x + i, y, color);
    }
}

static void test_vga_clear() {
    char* vga = (char*)TEST_VGA_ADDR;
    for (int i = 0; i < TEST_VGA_WIDTH * TEST_VGA_HEIGHT * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x0F;
    }
    test_current_row = 1;
}

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            test_vga_print("FAIL: ", 0, test_current_row, 0x0C); \
            test_vga_print(msg, 6, test_current_row, 0x0C); \
            test_current_row++; \
            return 0; \
        } \
    } while (0)

#define RUN_TEST(test_func, name) \
    do { \
        test_vga_print("Running ", 0, test_current_row, 0x0E); \
        test_vga_print(name, 8, test_current_row, 0x0E); \
        if (test_func()) { \
            test_vga_print("PASS", 70, test_current_row, 0x0A); \
        } else { \
            test_vga_print("FAIL", 70, test_current_row, 0x0C); \
        } \
        test_current_row++; \
    } while (0)

#define TEST_HEADER(title) \
    do { \
        test_vga_clear(); \
        test_vga_print("=== ", 0, 0, 0x0B); \
        test_vga_print(title, 4, 0, 0x0B); \
        test_vga_print(" ===", 4 + 18, 0, 0x0B); \
        test_current_row = 2; \
    } while (0)

#define TEST_FOOTER() \
    do { \
        test_vga_print("Test Suite Completed.", 0, test_current_row + 1, 0x0F); \
    } while (0)

#endif
