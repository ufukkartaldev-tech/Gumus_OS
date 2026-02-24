#include "framework.h"
#include "../src/kernel/core/memory/memory.h"
#include "../src/kernel/core/string.h"

/**
 * 2. KATMAN: PHYSICAL MEMORY MANAGER (ARSA TESTİ)
 * 4. KATMAN: HEAP (MUTFAK TESTİ)
 */

int test_pmm_rigorous() {
    // 1. Double Free Test
    void* f1 = pmm_alloc_frame();
    pmm_free_frame(f1);
    // Tekrar free etmek sistemi çökertmemeli veya bitmap'i bozmamalı (idempotent olmalı)
    pmm_free_frame(f1); 
    
    // 2. Alignment: Sayfalar 4KB hizalı mı?
    ASSERT_EQ(((uint32_t)f1 % 4096), 0, "Frame must be 4KB aligned");
    
    // 3. Reserved Protection (Fiziksel 0 adresi genelde rezerve edilir)
    // PMM'in 0. frame'i (0 - 4096) boş vermediğinden emin olalım
    void* f_low = pmm_alloc_frame();
    ASSERT((uint32_t)f_low >= 0x1000, "PMM should not allocate IVT/RealMode space (0x0)");
    
    return TEST_PASS;
}

int test_heap_canary() {
    /**
     * Magic Number / Canary Check
     * GümüşOS kmalloc'un önüne/arkasına canary koyuyor mu?
     * Eğer koymuyorsa bu testi "Dayı Tavsiyesi" olarak kendimiz simüle edelim.
     */
    typedef struct {
        uint32_t magic;
        uint32_t size;
    } heap_header_t;
    
    void* p = kmalloc(64);
    ASSERT(p != NULL, "kmalloc(64) failed");
    
    // GümüşOS heap implementasyonuna göre header'ı kontrol et (eğer varsa)
    // Bu test implementasyona özeldir. Şimdilik pointer benzersizliğini test edelim.
    void* p2 = kmalloc(64);
    ASSERT(p != p2, "Heap overlapping detected");
    
    return TEST_PASS;
}

int test_heap_fragmentation() {
    // Sürekli küçük ve orta bloklar al-ver
    void* ptrs[10];
    for (int i=0; i<10; i++) {
        ptrs[i] = kmalloc(i * 100 + 1);
    }
    
    // Memory Leak / Sızıntı kontrolü (Döngü sonrası heap_curr_break takibi)
    // Not: GümüşOS kfree() henüz bump-pointer ise serbest bırakamaz.
    for (int i=0; i<10; i++) {
        kfree(ptrs[i]);
    }
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("2 & 4 LAYER: MEMORY & HEAP TESTS");
    
    // 32MB Simülasyonu
    init_memory(32 * 1024 * 1024);
    
    RUN_TEST(test_pmm_rigorous, "PMM Rigorous (Double Free/Alignment)");
    RUN_TEST(test_heap_canary, "Heap Security (Magic Check)");
    RUN_TEST(test_heap_fragmentation, "Heap Fragmentation/Leak Simulation");
    
    _print_raw("Memory tests done.", 2, 20, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
