// GümüşOS C Kernel Unit Test Suite
// C kernel fonksiyonlarını test eder

#include <stddef.h>

// VGA adresleri
#define VGA_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// Test sonuçları
typedef enum {
    TEST_PASS,
    TEST_FAIL
} test_result_t;

// Test mesajları
static const char* MSG_START = "C Kernel Test Suite Baslatildi...";
static const char* MSG_VGA = "VGA Fonksiyonlari Testi: ";
static const char* MSG_MEMORY = "Bellek Fonksiyonlari Testi: ";
static const char* MSG_STRING = "String Fonksiyonlari Testi: ";
static const char* MSG_PASS = "PASS";
static const char* MSG_FAIL = "FAIL";
static const char* MSG_DONE = "C Kernel Testleri Tamamlandi!";

// VGA yardımcı fonksiyonları
static void vga_putchar(char c, int x, int y, char color) {
    char* vga = (char*)VGA_ADDR;
    vga[y * VGA_WIDTH * 2 + x * 2] = c;
    vga[y * VGA_WIDTH * 2 + x * 2 + 1] = color;
}

static void vga_print(const char* str, int x, int y, char color) {
    for (int i = 0; str[i]; i++) {
        vga_putchar(str[i], x + i, y, color);
    }
}

static void vga_clear_screen() {
    char* vga = (char*)VGA_ADDR;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x0F;
    }
}

// Bellek fonksiyonları
static void* memset_test(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

static int memcmp_test(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

// String fonksiyonları
static size_t strlen_test(const char* str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

static int strcmp_test(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char*)str1 - *(unsigned char*)str2;
}

// Test fonksiyonları
static test_result_t test_vga_functions() {
    vga_print(MSG_VGA, 0, 0, 0x0F);
    
    // Karakter yazma testi
    vga_putchar('A', 50, 0, 0x0A);
    char* vga = (char*)VGA_ADDR;
    if (vga[0 * VGA_WIDTH * 2 + 50 * 2] != 'A') {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Ekran temizleme testi
    vga_clear_screen();
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        if (vga[i] != ' ') {
            vga_print(MSG_FAIL, 50, 0, 0x0C);
            return TEST_FAIL;
        }
    }
    
    vga_print(MSG_PASS, 50, 0, 0x0A);
    return TEST_PASS;
}

static test_result_t test_memory_functions() {
    vga_print(MSG_MEMORY, 0, 1, 0x0F);
    
    // memset testi
    char buffer[100];
    memset_test(buffer, 'X', 10);
    for (int i = 0; i < 10; i++) {
        if (buffer[i] != 'X') {
            vga_print(MSG_FAIL, 50, 1, 0x0C);
            return TEST_FAIL;
        }
    }
    
    // memcmp testi
    char buffer2[10] = "XXXXXXXXXX";
    if (memcmp_test(buffer, buffer2, 10) != 0) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 1, 0x0A);
    return TEST_PASS;
}

static test_result_t test_string_functions() {
    vga_print(MSG_STRING, 0, 2, 0x0F);
    
    // strlen testi
    const char* test_str = "Hello";
    if (strlen_test(test_str) != 5) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // strcmp testi
    if (strcmp_test("Hello", "Hello") != 0) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    if (strcmp_test("Hello", "World") == 0) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 2, 0x0A);
    return TEST_PASS;
}

// Ana test fonksiyonu
void kernel_main() {
    vga_clear_screen();
    vga_print(MSG_START, 0, 0, 0x0F);
    
    // VGA testleri
    test_result_t result = test_vga_functions();
    
    // Bellek testleri
    result = test_memory_functions();
    
    // String testleri
    result = test_string_functions();
    
    // Test sonu
    vga_print(MSG_DONE, 0, 4, 0x0F);
    
    // Sonsuz döngü
    while (1) {
        __asm__ volatile("hlt");
    }
}
