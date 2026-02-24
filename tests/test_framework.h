#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>

/**
 * GümüşOS Test Framework (Zırhlı v2)
 * ---------------------------------
 */

#define TEST_VGA_ADDR 0xB8000
#define TEST_VGA_WIDTH 80
#define TEST_VGA_HEIGHT 25

// Dayı Tavsiyesi 3: Statik değişkenlerin çakışmaması için 
// Her test dosyası kendi kopyasını kullansın (Standalone build uyumlu)
static int test_current_row = 1;

// Dayı Tavsiyesi 1: volatile olmazsa olmaz!
static void test_vga_putchar(char c, int x, int y, char color) {
    // Sınır Kontrolü (Dayı Tavsiyesi 2)
    if (x < 0 || x >= TEST_VGA_WIDTH || y < 0 || y >= TEST_VGA_HEIGHT) return;

    volatile uint16_t* vga = (volatile uint16_t*)TEST_VGA_ADDR;
    vga[y * TEST_VGA_WIDTH + x] = (uint16_t)c | ((uint16_t)color << 8);
}

static void test_vga_print(const char* str, int x, int y, char color) {
    for (int i = 0; str[i]; i++) {
        // Dayı Tavsiyesi 2: Taşma kontrolü
        if (x + i >= TEST_VGA_WIDTH) break; 
        test_vga_putchar(str[i], x + i, y, color);
    }
}

// Dayı Tavsiyesi 4: 4 byte'lık kopyalama ile hızlanma (Hamallığı bırak)
static void test_vga_clear() {
    volatile uint32_t* vga32 = (volatile uint32_t*)TEST_VGA_ADDR;
    uint32_t blank = (0x0F200F20); // İki tane ' ' (0x20) ve Beyaz (0x0F)
    
    for (int i = 0; i < (TEST_VGA_WIDTH * TEST_VGA_HEIGHT * 2) / 4; i++) {
        vga32[i] = blank;
    }
    test_current_row = 1;
}

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            test_vga_print("[FAIL] ", 0, test_current_row, 0x0C); \
            test_vga_print(msg, 7, test_current_row, 0x0F); \
            test_current_row++; \
            return 0; \
        } \
    } while (0)

#define RUN_TEST(test_func, name) \
    do { \
        test_vga_print("Running: ", 0, test_current_row, 0x07); \
        test_vga_print(name, 9, test_current_row, 0x0F); \
        if (test_func()) { \
            test_vga_print("[PASS]", 72, test_current_row, 0x0A); \
        } else { \
            test_vga_print("[FAIL]", 72, test_current_row, 0x0C); \
        } \
        test_current_row++; \
    } while (0)

#define TEST_HEADER(title) \
    do { \
        test_vga_clear(); \
        test_vga_print("--- GUMUS OS CORE TEST SUITE ---", 24, 0, 0x0B); \
        test_vga_print(title, 2, 2, 0x0E); \
        test_current_row = 4; \
    } while (0)

#define TEST_FOOTER() \
    do { \
        test_vga_print("------------------------------------------------", 0, test_current_row++, 0x07); \
        test_vga_print("Test Suite Execution Finished.", 0, test_current_row, 0x0A); \
    } while (0)

#endif
