#include "framework.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * 3. KATMAN: ADVANCED PAGING & VMM (DIRTY/ACCESSED & TLB)
 */

// Sayfa Tablosu Kaydı (PTE) bulucu
// NOT: GümüşOS şu an Identity Map (0-16MB) kullandığı için 
// kernel_directory->tables dizisindeki adreslere doğrudan erişebiliyoruz.
static page_entry_t* get_pte(uint32_t virt) {
    if (!kernel_directory) return NULL;
    
    uint32_t pd_idx = virt >> 22;
    uint32_t pt_idx = (virt >> 12) & 0x03FF;
    
    if (!(kernel_directory->tablesPhysical[pd_idx] & 1)) return NULL;
    
    page_table_t* table = kernel_directory->tables[pd_idx];
    if (!table) return NULL;
    
    return &table->pages[pt_idx];
}

int test_tapu_detaylari() {
    uint32_t addr = 0x100000; // 1MB
    page_entry_t* entry = get_pte(addr);
    
    ASSERT(entry != NULL, "PTE entry not found");
    ASSERT(entry->present == 1, "Page not present");
    ASSERT(entry->rw == 1, "Page not writable");
    ASSERT(entry->user == 0, "Security Violation: Kernel page marked as USER!");

    return TEST_PASS;
}

int test_accessed_dirty_bits() {
    /**
     * Strateji: Bir sayfa oluştur, ona oku/yaz yap ve donanımın 
     * otomatik olarak A ve D bitlerini set edip etmediğine bak.
     */
    void* phys = pmm_alloc_frame();
    volatile uint32_t* virt = (uint32_t*)0xDEADD000;
    
    // Map as RW, Present
    map_page(phys, (void*)virt, 3);
    
    page_entry_t* entry = get_pte((uint32_t)virt);
    ASSERT(entry != NULL, "Mapping failed");
    
    // Başlangıçta bitler 0 olmalı (veya temizlenmeli)
    entry->accessed = 0;
    entry->dirty = 0;
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");

    // 1. ACCESSED TESTİ (Read)
    uint32_t dummy = *virt; (void)dummy;
    ASSERT(entry->accessed == 1, "Accessed bit failed to trigger on READ");
    
    // 2. DIRTY TESTİ (Write)
    *virt = 0xCAFEBABE;
    ASSERT(entry->dirty == 1, "Dirty bit failed to trigger on WRITE");
    
    return TEST_PASS;
}

int test_tlb_invalidation() {
    void* phys1 = pmm_alloc_frame();
    void* phys2 = pmm_alloc_frame();
    volatile uint32_t* virt = (uint32_t*)0xDEADE000;
    
    // İlk harita: phys1
    map_page(phys1, (void*)virt, 3);
    *virt = 0x11111111;
    
    // Map değiştir: phys2
    // map_page içerisindeki invlpg sayesinde CPU yeni adresi görmeli
    map_page(phys2, (void*)virt, 3);
    
    // Eğer TLB flush yapılmazsa CPU hala phys1'e bakıp 0x11111111 görebilir!
    ASSERT(*virt != 0x11111111, "TLB Flush failed: CPU still sees old page content");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("3. LAYER: PAGING TAPU & DYNAMICS");
    
    init_memory(32 * 1024 * 1024);
    
    RUN_TEST(test_tapu_detaylari, "PTE Security (U/S Bit)");
    RUN_TEST(test_accessed_dirty_bits, "Hardware Bits (Accessed/Dirty)");
    RUN_TEST(test_tlb_invalidation, "TLB Consistency (invlpg check)");
    
    _print_raw("Advanced Paging dynamics verified.", 2, 20, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
