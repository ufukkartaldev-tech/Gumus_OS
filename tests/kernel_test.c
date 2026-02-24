#include "framework.h"
#include "../src/kernel/core/string.h"
#include "../src/kernel/core/math.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * KERNEL CORE TESTS (GELİŞMİŞ & ZIRHLI)
 * ------------------------------------
 * Bu testler kernel'ın temel yapı taşlarını zorlar.
 */

// Dayı Tavsiyesi: FPU (Floating Point Unit) Aktifleştirme
// x86'da double kullanmadan önce bu vana açılmalı!
void init_fpu() {
    uint32_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // EM bitini temizle (Emulation kapalı)
    cr0 |= (1 << 1);  // MP bitini set et (Monitor Coprocessor)
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (3 << 9);  // OSFXSR ve OSXMMEXCPT bitlerini aç (Streaming SIMD desteği için)
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));

    __asm__ volatile("finit"); // FPU'yu sıfırla
}

int test_string_robustness() {
    // 1. strlen(NULL) koruması
    ASSERT_EQ(strlen(NULL), 0, "strlen(NULL) support");
    
    // 2. strcmp mismatch
    ASSERT(strcmp("Gumus", "GumusOS") != 0, "strcmp mismatch length");
    ASSERT(strncmp("GumusOS", "Gumus", 5) == 0, "strncmp first 5");
    
    return TEST_PASS;
}

int test_memory_guards() {
    uint8_t buffer[12] = {0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF};
    uint8_t* target = buffer + 1; // [0] ve [11] guard byte'lar
    
    // Dayı Tavsiyesi 2: Sınırların dışına tasma kontrolü (Guard Bytes)
    memset(target, 'X', 10);
    
    ASSERT_EQ(buffer[0], 0xFF, "memset prefix guard corruption");
    ASSERT_EQ(buffer[11], 0xFF, "memset suffix guard corruption");
    ASSERT_EQ(buffer[1], 'X', "memset first byte");
    ASSERT_EQ(buffer[10], 'X', "memset last byte");
    
    return TEST_PASS;
}

int test_unaligned_memory_ops() {
    uint8_t data[20];
    for(int i=0; i<20; i++) data[i] = 0;
    
    // 1. Unaligned memset (Adres 4'ün katı değil)
    uint8_t* unaligned_dst = data + 1;
    memset(unaligned_dst, 0xAA, 7);
    
    ASSERT_EQ(data[0], 0, "unaligned memset prefix security");
    ASSERT_EQ(data[1], 0xAA, "unaligned memset start");
    ASSERT_EQ(data[7], 0xAA, "unaligned memset end");
    ASSERT_EQ(data[8], 0, "unaligned memset suffix security");
    
    // 2. Unaligned memcpy (Kaynak ve hedef farklı hizalarda)
    uint8_t src[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    uint8_t dst[10] = {0};
    memcpy(dst + 1, src + 2, 5); // Dest alignment mismatch
    
    ASSERT_EQ(dst[0], 0, "memcpy unaligned prefix");
    ASSERT_EQ(dst[1], 3, "memcpy unaligned data match 1");
    ASSERT_EQ(dst[5], 7, "memcpy unaligned data match 5");
    
    return TEST_PASS;
}

int test_math_fpu_safety() {
    // Eğer buraya kadar FPU hatası almadıysak vana açılmıştır
    double val = sqrt(16.0);
    ASSERT_EQ((uint32_t)val, 4, "sqrt(16.0) with FPU enabled");
    
    double s = sin(0.0);
    int32_t s_int = (int32_t)(s * 1000); // 0.000...
    ASSERT_EQ(s_int, 0, "sin(0.0) precision check");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("KERNEL CORE RIGOROUS TESTS");
    
    // Dayı Tavsiyesi 1: FPU açılmazsa math testleri sistemi kilitler!
    init_fpu();
    
    RUN_TEST(test_string_robustness, "String Protection (NULL & Length)");
    RUN_TEST(test_memory_guards, "Memory Boundary Guards");
    RUN_TEST(test_unaligned_memory_ops, "Unaligned Memory Access (R/W)");
    RUN_TEST(test_math_fpu_safety, "FPU & Floating Point Safety");
    
    _print_raw("Kernel tests completed successfully.", 2, 22, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
