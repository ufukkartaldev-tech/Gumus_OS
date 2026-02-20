#include "memory.h"
#include "kernel.h"
#include "string.h" // memset için

// --- PMM (Physical Memory Manager) ---
// Basit Bitmap Yöntemi (32MB RAM varsayalım şimdilik)
#define MEM_SIZE_MAX 0x2000000 // 32MB
#define FRAMES_COUNT (MEM_SIZE_MAX / PAGE_SIZE)

static uint32_t frames[FRAMES_COUNT / 32];
static uint32_t used_frames = 0;

void pmm_set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frames[idx] |= (1 << off);
    used_frames++;
}

void pmm_clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    frames[idx] &= ~(1 << off);
    used_frames--;
}

uint32_t pmm_first_free_frame() {
    for (uint32_t i = 0; i < FRAMES_COUNT / 32; i++) {
        if (frames[i] != 0xFFFFFFFF) {
            for (int j = 0; j < 32; j++) {
                if (!(frames[i] & (1 << j))) {
                    return i * 32 + j;
                }
            }
        }
    }
    return -1;
}

void* pmm_alloc_frame() {
    uint32_t frame = pmm_first_free_frame();
    if (frame == (uint32_t)-1) return 0; // OOM
    pmm_set_frame(frame * PAGE_SIZE);
    return (void*)(frame * PAGE_SIZE);
}

// --- VMM (Virtual Memory Manager) ---
page_directory_t* kernel_directory = 0;
page_directory_t* current_directory = 0;

void switch_page_directory(page_directory_t* dir) {
    current_directory = dir;
    
    // tablesPhysical dizisinin fiziksel adresini bul
    // Identity map alanındaysak (Kernel) direkt kendisi.
    // Heap'teyse get_phys_addr kullanmalıyız.
    uint32_t phys = (uint32_t)get_phys_addr(&dir->tablesPhysical);
    if (!phys) phys = (uint32_t)&dir->tablesPhysical; // Henüz paging yoksa
    
    asm volatile("mov %0, %%cr3":: "r"(phys));
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // PG Bit
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void map_page_in_dir(page_directory_t* dir, void* phys, void* virt, uint32_t flags) {
    uint32_t pd_idx = (uint32_t)virt >> 22;
    uint32_t pt_idx = ((uint32_t)virt >> 12) & 0x03FF;
    
    // Page Table var mı?
    if (!(dir->tablesPhysical[pd_idx] & 1)) {
        // Yoksa oluştur (Fiziksel bellekten bir sayfa al)
        void* new_table_phys = pmm_alloc_frame();
        
        // Paging aktifse ve current_directory kernel_directory değilse, 
        // new_table_phys adresine doğrudan erişemeyebiliriz!
        // Ancak biz kernel'ı identity map yaptığımız için (0-16MB) 
        // ve PMM 1MB-32MB arasında çalıştığı için çoğu zaman erişebiliriz.
        // Ama garantilemek için identity mapping alanında kalmalıyız.
        
        dir->tablesPhysical[pd_idx] = (uint32_t)new_table_phys | 0x7; // Present, RW, User
        dir->tables[pd_idx] = (page_table_t*)new_table_phys; // Virtual = Physical (Identity)
        
        memset(new_table_phys, 0, 4096);
    }
    
    page_table_t* table = dir->tables[pd_idx];
    page_entry_t* page = &table->pages[pt_idx];
    
    page->frame = (uint32_t)phys >> 12;
    page->present = 1;
    page->rw = (flags & 2) ? 1 : 0;
    page->user = (flags & 4) ? 1 : 0;
}

void map_page(void* phys, void* virt, uint32_t flags) {
    map_page_in_dir(kernel_directory, phys, virt, flags);
}

page_directory_t* create_process_directory() {
    page_directory_t* dir = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), 4096);
    memset(dir, 0, sizeof(page_directory_t));
    
    // Kernel'ı kopyala (Identity Map 0-16MB)
    // kernel_directory'nin ilk 4 girişi 16MB eder (4 * 4MB)
    for (int i = 0; i < 4; i++) {
        dir->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
        dir->tables[i] = kernel_directory->tables[i];
    }
    
    return dir;
}

void init_paging() {
    // 1. PMM Başlat (Tüm belleği boş işaretle)
    memset(frames, 0, sizeof(frames));
    
    // 2. Kernel Directory Oluştur (Statik veya PMM'den)
    // PMM'den alalım. Şu an paging kapalı, fiziksel adres = sanal adres.
    kernel_directory = (page_directory_t*)pmm_alloc_frame();
    memset(kernel_directory, 0, sizeof(page_directory_t));
    
    // 3. Identity Map (İlk 4MB) - Kernel ve VGA Buffer buraya sığar
    // Daha güvenli olması için ilk 8MB veya 16MB mapleyelim.
    // 0x00000000 - 0x01000000 (16MB) -> Identity
    
    for (uint32_t i = 0; i < 0x1000000; i += PAGE_SIZE) {
        map_page((void*)i, (void*)i, 3); // Kernel RW
        // Bu bitmap'te de işaretlenmeli!
        pmm_set_frame(i);
    }
    
    // 4. Heap için ayrılan alanı (KERNEL_HEAP_START) da mapleyelim mi?
    // Hayır, on-demand (malloc çağrıldığında) maplenecek.
    
    // 5. Paging Aktifleştir
    switch_page_directory(kernel_directory);
    
    print("\n[VMM] Paging Aktif. (Identity Map 0-16MB)\n");
}

void* get_phys_addr(void* virt) {
    if (!current_directory) return virt; // Paging yoksa
    
    uint32_t pd_idx = (uint32_t)virt >> 22;
    uint32_t pt_idx = ((uint32_t)virt >> 12) & 0x03FF;
    
    if (!(current_directory->tablesPhysical[pd_idx] & 1)) return 0;
    
    page_table_t* table = current_directory->tables[pd_idx];
    page_entry_t* page = &table->pages[pt_idx];
    
    if (!page->present) return 0;
    
    return (void*)((page->frame << 12) + ((uint32_t)virt & 0xFFF));
}

// --- Kernel Heap (Kmalloc) ---
// Paging sonrası Sanal Adres uzayını yöneten basit bir allocator

#define HEAP_START_VIRT 0xC0000000
static uint32_t heap_curr_break = HEAP_START_VIRT;

void* kmalloc_aligned(size_t size, uint32_t align) {
    if (heap_curr_break % align != 0) {
        heap_curr_break += (align - (heap_curr_break % align));
    }
    return kmalloc(size);
}

void* kmalloc(size_t size) {
    // 4 byte align
    if (size % 4 != 0) size += (4 - (size % 4));
    
    void* addr = (void*)heap_curr_break;
    
    // Gerekli sayfalar mapli mi?
    uint32_t start_page = (uint32_t)addr & 0xFFFFF000;
    uint32_t end_page = ((uint32_t)addr + size) & 0xFFFFF000;
    
    for (uint32_t pg = start_page; pg <= end_page; pg += PAGE_SIZE) {
        if (!get_phys_addr((void*)pg)) {
            void* phys = pmm_alloc_frame();
            if (!phys) return 0; // OOM
            map_page(phys, (void*)pg, 3); // RW Kernel
        }
    }
    
    heap_curr_break += size;
    return addr;
}

void kfree(void* ptr) {
    // Bump pointer'da free zordur.
    // Şimdilik boş işlem.
    (void)ptr;
}

void init_memory(uint32_t mem_size) {
    (void)mem_size; // Otomatik algıla veya sabit kullan
    init_paging();
}
