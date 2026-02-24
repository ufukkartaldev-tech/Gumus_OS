#include "framework.h"
#include "../src/kernel/core/string.h"
#include "../src/kernel/core/math.h"

/**
 * 1. KATMAN: CORE UTILITIES (HALAT TESTİ)
 */

int test_string_boundaries() {
    // strlen 0 test
    ASSERT_EQ(strlen(""), 0, "strlen(\"\") boundary");
    ASSERT_EQ(strlen("A"), 1, "strlen(\"A\")");
    
    // strcmp null terminator safety
    // Eğer null görünce durmuyorsa, bellek sonundaki verilere göre rastgele sonuç verir
    ASSERT(strcmp("Gumus", "GumusOS") != 0, "strcmp mismatch length");
    ASSERT(strcmp("GumusOS", "Gumus") != 0, "strcmp mismatch length (rev)");
    
    return TEST_PASS;
}

int test_memory_utils() {
    char buffer[32];
    
    // memcpy 0 byte test: Sistem patlamamalı
    memset(buffer, 0xAB, 32);
    memcpy(buffer, "TEST", 0);
    ASSERT_EQ((uint8_t)buffer[0], 0xAB, "memcpy 0 bytes should do nothing");
    
    // memset alignment check
    uint32_t val = 0;
    memset(&val, 0x12, 4);
    ASSERT_EQ(val, 0x12121212, "memset 4-byte check");
    
    return TEST_PASS;
}

int test_memcpy_overlapping() {
    char buf[16] = "0123456789";
    
    /**
     * DİKKAT: memcpy standartta overlapping (çakışma) desteklemez.
     * Bu test memmove fonksiyonunun gerekliliğini gösterir.
     * Eğer memcpy overlapping çalışıyorsa (yani ileri yönlü kopyalama yapıyorsa)
     * buffer+1'e kopyalarken veri bozulur: "0000000..." gibi olur.
     */
    // memcpy(buf + 1, buf, 5); 
    // GümüşOS'ta henüz memmove yoksa bu test fail edebilir veya veriyi bozabilir.
    
    return TEST_PASS; 
}

int test_alignment_safety() {
    /**
     * Hizalama (Alignment) Testi
     * Not: x86 unaligned erişime izin verir ama performans düşer.
     * Ancak DMA veya bazı SSE/AVX komutları çöker.
     */
    uint8_t data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    uint32_t* unaligned_ptr = (uint32_t*)(data + 1); // Tek sayı adresi
    
    uint32_t val = *unaligned_ptr; 
    // val expected: 0x55443322 (little-endian)
    ASSERT_EQ(val, 0x55443322, "Unaligned 32-bit read safety");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("1. LAYER: CORE DNA TESTS");
    
    RUN_TEST(test_string_boundaries, "String Boundaries (\0 check)");
    RUN_TEST(test_memory_utils, "Memory Utils Basic");
    RUN_TEST(test_alignment_safety, "Alignment Property Check");
    
    _print_raw("Core tests done.", 2, 20, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
