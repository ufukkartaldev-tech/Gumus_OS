#include "framework.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * 2. KATMAN: MEMORY MANAGEMENT (TAPU DAİRESİ) - RIGOROUS v2
 */

// Kernel sınırlarını kernel.h veya linkerdan almalıyız ama şimdilik sembolik:
#define KERNEL_LOAD_ADDR 0x100000 
#define KERNEL_SIZE_APPROX 0x100000 // 1MB

int test_pmm_rigorous() {
    // 1. Double Free & Idempotency (Dayı Tavsiyesi 1)
    void* f1 = pmm_alloc_frame();
    pmm_free_frame(f1);
    pmm_free_frame(f1); // İkinci free sistemi bozmamalı!
    
    void* f2 = pmm_alloc_frame();
    ASSERT_EQ(f1, f2, "PMM should reuse frame after double free check");
    
    // 2. Reserved Space Check (Dayı Tavsiyesi 2)
    void* f3 = pmm_alloc_frame();
    uint32_t addr = (uint32_t)f3;
    ASSERT(addr >= 0x100000, "PMM allocated BIOS reserved space (<1MB)!");
    // Kernel alanını da korumalı (Identity map 0-16MB ise)
    ASSERT(addr >= 0x1000000 || addr < 0x100000, "PMM allocated kernel space!");
    
    pmm_free_frame(f2);
    pmm_free_frame(f3);
    return TEST_PASS;
}

int test_heap_canary_and_coalescing() {
    // 1. Heap Canary (Dayı Tavsiyesi 3)
    // kmalloc başlığı (struct heap_block) 16 boyundadır. Magic ilk 4 byte.
    void* p1 = kmalloc(32);
    uint32_t* magic_ptr = (uint32_t*)((uint8_t*)p1 - 16);
    ASSERT_EQ(*magic_ptr, 0xCAFEBABE, "Heap Header Canary (Magic) corrupted or missing!");
    
    // 2. Coalescing & Fragmentation (Dayı Tavsiyesi 4)
    void* p2 = kmalloc(32);
    void* p3 = kmalloc(32);
    
    kfree(p1);
    kfree(p2);
    kfree(p3);
    
    // p1, p2, p3 birleşmiş olmalı. Şimdi 100 byte yer isteyelim. 
    // Eğer birleşmediyse (coalesce yoksa) yeni bir sayfa açar veya OOM verir.
    void* p_big = kmalloc(90); 
    // p_big, p1'in eski yerinde (veya çok yakınında) olmalı
    ASSERT_EQ(p_big, p1, "Heap coalescing failed: neighbor blocks did not merge!");
    
    kfree(p_big);
    return TEST_PASS;
}

int test_heap_leak_stress() {
    // Sürekli al-ver yaparak RAM bitiyor mu bak.
    void* ptrs[100];
    for(int i=0; i<100; i++) {
        ptrs[i] = kmalloc(128);
        ASSERT(ptrs[i] != 0, "Heap Out of Memory in stress test!");
    }
    
    for(int i=0; i<100; i++) {
        kfree(ptrs[i]);
    }
    
    // Tüm RAM geri dönmüş olmalı. Büyük bir yer isteyelim.
    void* large = kmalloc(1024 * 16);
    ASSERT(large != 0, "Memory leak detected: Large allocation failed after kfree stress");
    
    kfree(large);
    return TEST_PASS;
}

void kernel_main() {
    test_header("2. LAYER: MEMORY INFRASTRUCTURE TESTS");
    
    // PMM/VMM initialization usually happens in boot, here we ensure it's ready
    // init_memory(32 * 1024 * 1024);
    
    RUN_TEST(test_pmm_rigorous, "PMM Double-Free & Reserved Protection");
    RUN_TEST(test_heap_canary_and_coalescing, "Heap Canary & Coalescing Check");
    RUN_TEST(test_heap_leak_stress, "Heap Stress & Leak Check");
    
    _print_raw("Memory subsystem is healthy.", 2, 21, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
