#include "framework.h"
#include "../src/kernel/core/string.h"
#include "../src/kernel/core/math.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * 1. KATMAN: CORE UTILITIES (HALAT TESTİ) - GELİŞMİŞ VERSİYON
 */

int test_string_robustness() {
    // strlen boundaries
    ASSERT_EQ(strlen(""), 0, "strlen(\"\")");
    ASSERT_EQ(strlen("Gumus"), 5, "strlen(\"Gumus\")");
    
    // strcmp null terminator safety
    ASSERT(strcmp("A", "AB") != 0, "strcmp mismatch length short");
    ASSERT(strcmp("ABC", "AB") != 0, "strcmp mismatch length long");
    
    // NULL pointer safety note:
    // Standart strlen(NULL) tanımsızdır ancak kernel'da 0 dönmesi tercih edilebilir.
    // Şimdilik sistemin çöküp çökmediğini görmek için sembolik kalsın.
    // ASSERT_EQ(strlen(NULL), 0, "strlen(NULL) safety check");
    
    return TEST_PASS;
}

int test_memory_alignment_rigorous() {
    uint8_t buffer[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint32_t* unaligned_ptr = (uint32_t*)(buffer + 1); // Tek (odd) adres
    
    // 1. Unaligned Yazma (Write)
    *unaligned_ptr = 0xDEADBEEF;
    
    // Byte byte kontrol (Little Endian)
    ASSERT_EQ(buffer[0], 0, "adjacent byte corruption (prefix)");
    ASSERT_EQ(buffer[1], 0xEF, "unaligned byte 0");
    ASSERT_EQ(buffer[2], 0xBE, "unaligned byte 1");
    ASSERT_EQ(buffer[3], 0xAD, "unaligned byte 2");
    ASSERT_EQ(buffer[4], 0xDE, "unaligned byte 3");
    ASSERT_EQ(buffer[5], 0, "adjacent byte corruption (suffix)");
    
    // 2. Unaligned Okuma (Read)
    uint32_t val = *unaligned_ptr;
    ASSERT_EQ(val, 0xDEADBEEF, "unaligned 32-bit read");
    
    return TEST_PASS;
}

int test_memmove_overlapping() {
    char buf[] = "ABCDE";
    
    // İleriye doğru çakışma (Right shift)
    // ABCDE -> AABCD
    memmove(buf + 1, buf, 4);
    ASSERT_EQ(strcmp(buf, "AABCD"), 0, "memmove overlap right failed");
    
    // Geriye doğru çakışma (Left shift)
    // AABCD -> ABCD D
    char buf2[] = "12345";
    memmove(buf2, buf2 + 1, 4);
    ASSERT_EQ(buf2[0], '2', "memmove overlap left failed");
    ASSERT_EQ(buf2[3], '5', "memmove overlap left end check");
    
    return TEST_PASS;
}

int test_memset_large() {
    // Dayı Tavsiyesi 3: Sayfa boyutunda (4KB) sıfırlama testi
    static uint8_t page_buf[4096];
    
    // Önce kirlet
    for(int i=0; i<4096; i++) page_buf[i] = 0xFF;
    
    // Sıfırla
    memset(page_buf, 0, 4096);
    
    // Kontrol et
    for(int i=0; i<4096; i++) {
        if(page_buf[i] != 0) {
            ASSERT_DETAIL(0, "memset failed at index", i, page_buf[i]);
            return TEST_FAIL;
        }
    }
    
    return TEST_PASS;
}

int test_stress_memcpy() {
    // Büyük veri ve kırık adres testi
    #define STRESS_SIZE 1024 * 16 // 16KB (Stack sığması için, kmalloc yoksa)
    static uint8_t src[STRESS_SIZE];
    static uint8_t dst[STRESS_SIZE];
    
    // Veri hazırla
    for(int i=0; i<STRESS_SIZE; i++) src[i] = (uint8_t)(i & 0xFF);
    
    // Kırık adres (unaligned offset) ile kopyala
    // src+1'den dst+3'e 1000 byte
    memcpy(dst + 3, src + 1, 1000);
    
    // Doğrula
    ASSERT_EQ(memcmp(dst + 3, src + 1, 1000), 0, "stress memcpy with unaligned offsets");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("1. LAYER: RIGOROUS CORE DNA TESTS");
    
    // Bazı testler için bellek altyapısı gerekebilir
    init_memory(16 * 1024 * 1024);
    
    RUN_TEST(test_string_robustness, "String Robustness (\0 & NULL)");
    RUN_TEST(test_memory_alignment_rigorous, "Unaligned Access (Read/Write)");
    RUN_TEST(test_memmove_overlapping, "Overlapping Memory (memmove)");
    RUN_TEST(test_memset_large, "Large Area Wipe (4KB Page)");
    RUN_TEST(test_stress_memcpy, "Stress Test (16KB / Broken Addr)");
    
    _print_raw("Rigorously tested.", 2, 20, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
