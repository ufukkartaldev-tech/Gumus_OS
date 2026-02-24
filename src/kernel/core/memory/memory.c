#include "memory.h"
#include "kernel.h"
#include "string.h" // memset iÃ§in

// --- PMM (Physical Memory Manager) ---
// Basit Bitmap YÃ¶ntemi (32MB RAM varsayalÄ±m ÅŸimdilik)
#define MEM_SIZE_MAX 0x2000000 // 32MB
#define FRAMES_COUNT (MEM_SIZE_MAX / PAGE_SIZE)

static uint32_t frames[FRAMES_COUNT / 32];
static uint32_t used_frames = 0;

void pmm_set_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    if (!(frames[idx] & (1 << off))) {
        frames[idx] |= (1 << off);
        used_frames++;
    }
}

void pmm_clear_frame(uint32_t frame_addr) {
    uint32_t frame = frame_addr / PAGE_SIZE;
    uint32_t idx = frame / 32;
    uint32_t off = frame % 32;
    if (frames[idx] & (1 << off)) {
        frames[idx] &= ~(1 << off);
        used_frames--;
    }
}

void pmm_free_frame(void* frame) {
    pmm_clear_frame((uint32_t)frame);
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
    // Identity map alanÄ±ndaysak (Kernel) direkt kendisi.
    // Heap'teyse get_phys_addr kullanmalÄ±yÄ±z.
    uint32_t phys = (uint32_t)get_phys_addr(&dir->tablesPhysical);
    if (!phys) phys = (uint32_t)&dir->tablesPhysical; // HenÃ¼z paging yoksa
    
    asm volatile("mov %0, %%cr3":: "r"(phys));
    
    uint32_t cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // PG Bit
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

void map_page_in_dir(page_directory_t* dir, void* phys, void* virt, uint32_t flags) {
    uint32_t pd_idx = (uint32_t)virt >> 22;
    uint32_t pt_idx = ((uint32_t)virt >> 12) & 0x03FF;
    
    // Page Table var mÄ±?
    if (!(dir->tablesPhysical[pd_idx] & 1)) {
        // Yoksa oluÅŸtur (Fiziksel bellekten bir sayfa al)
        void* new_table_phys = pmm_alloc_frame();
        
        // Paging aktifse ve current_directory kernel_directory deÄŸilse, 
        // new_table_phys adresine doÄŸrudan eriÅŸemeyebiliriz!
        // Ancak biz kernel'Ä± identity map yaptÄ±ÄŸÄ±mÄ±z iÃ§in (0-16MB) 
        // ve PMM 1MB-32MB arasÄ±nda Ã§alÄ±ÅŸtÄ±ÄŸÄ± iÃ§in Ã§oÄŸu zaman eriÅŸebiliriz.
        // Ama garantilemek iÃ§in identity mapping alanÄ±nda kalmalÄ±yÄ±z.
        
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

    // TLB Flush: Değişikliği işlemciye bildir
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

void map_page(void* phys, void* virt, uint32_t flags) {
    map_page_in_dir(kernel_directory, phys, virt, flags);
}

page_directory_t* create_process_directory() {
    page_directory_t* dir = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), 4096);
    memset(dir, 0, sizeof(page_directory_t));
    
    // 1. Identity Map Kopyala (0-16MB)
    for (int i = 0; i < 4; i++) {
        dir->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
        dir->tables[i] = kernel_directory->tables[i];
    }
    
    // 2. Higher Half Kopyala (3GB - 4GB)
    // 3GB = 768. index
    for (int i = 768; i < 1024; i++) {
        dir->tablesPhysical[i] = kernel_directory->tablesPhysical[i];
        dir->tables[i] = kernel_directory->tables[i];
    }
    
    return dir;
}

void init_paging() {
    // 1. PMM BaÅŸlat (TÃ¼m belleÄŸi boÅŸ iÅŸaretle)
    memset(frames, 0, sizeof(frames));
    
    // 2. Kernel Directory OluÅŸtur (Statik veya PMM'den)
    // PMM'den alalÄ±m. Åu an paging kapalÄ±, fiziksel adres = sanal adres.
    kernel_directory = (page_directory_t*)pmm_alloc_frame();
    memset(kernel_directory, 0, sizeof(page_directory_t));
    
    // 3. Identity Map (Ä°lk 4MB) - Kernel ve VGA Buffer buraya sÄ±ÄŸar
    // Daha gÃ¼venli olmasÄ± iÃ§in ilk 8MB veya 16MB mapleyelim.
    // 0x00000000 - 0x01000000 (16MB) -> Identity
    
    for (uint32_t i = 0; i < 0x1000000; i += PAGE_SIZE) {
        map_page((void*)i, (void*)i, 3); // Kernel RW
        // Bu bitmap'te de iÅŸaretlenmeli!
        pmm_set_frame(i);
    }
    
    // 4. Heap iÃ§in ayrÄ±lan alanÄ± (KERNEL_HEAP_START) da mapleyelim mi?
    // HayÄ±r, on-demand (malloc Ã§aÄŸrÄ±ldÄ±ÄŸÄ±nda) maplenecek.
    
    // 5. Paging AktifleÅŸtir
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

// --- Kernel Heap (Boundary Tag Allocator) ---
#define HEAP_MAGIC 0xCAFEBABE
#define HEAP_START_VIRT 0xC0000000

typedef struct heap_block {
    uint32_t magic;
    uint32_t size;    // Payload size
    int is_free;
    struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_start = (heap_block_t*)HEAP_START_VIRT;
static uint32_t heap_curr_break = HEAP_START_VIRT;

static void* expand_heap(size_t size) {
    void* addr = (void*)heap_curr_break;
    uint32_t needed_size = size + sizeof(heap_block_t);
    
    // Align to page size
    if (needed_size % PAGE_SIZE != 0) 
        needed_size += (PAGE_SIZE - (needed_size % PAGE_SIZE));

    for (uint32_t i = 0; i < needed_size; i += PAGE_SIZE) {
        void* phys = pmm_alloc_frame();
        if (!phys) return 0;
        map_page(phys, (void*)(heap_curr_break + i), 3);
    }
    
    heap_block_t* block = (heap_block_t*)heap_curr_break;
    block->magic = HEAP_MAGIC;
    block->size = needed_size - sizeof(heap_block_t);
    block->is_free = 1;
    block->next = 0;
    
    heap_curr_break += needed_size;
    return block;
}

void* kmalloc(size_t size) {
    if (size == 0) return 0;
    // Align size to 4 bytes
    if (size % 4 != 0) size += (4 - (size % 4));

    if (heap_curr_break == HEAP_START_VIRT) {
        expand_heap(size);
    }

    heap_block_t* curr = heap_start;
    heap_block_t* prev = 0;

    while (curr) {
        if (curr->is_free && curr->size >= size) {
            // Split block if possible
            if (curr->size >= size + sizeof(heap_block_t) + 4) {
                heap_block_t* next_block = (heap_block_t*)((uint32_t)curr + sizeof(heap_block_t) + size);
                next_block->magic = HEAP_MAGIC;
                next_block->size = curr->size - size - sizeof(heap_block_t);
                next_block->is_free = 1;
                next_block->next = curr->next;
                
                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = 0;
            return (void*)((uint32_t)curr + sizeof(heap_block_t));
        }
        prev = curr;
        curr = curr->next;
    }

    // No free block found, expand
    heap_block_t* new_block = expand_heap(size);
    if (!new_block) return 0;
    
    // Link the new block
    if (prev) prev->next = new_block;
    
    return kmalloc(size); // Try again
}

void* kmalloc_aligned(size_t size, uint32_t align) {
    // Basic aligned kmalloc: if align is 4096, just give a whole page
    if (align == 4096) {
        void* ptr = kmalloc(size + 4096);
        uint32_t addr = (uint32_t)ptr;
        if (addr % 4096 != 0) {
            addr += (4096 - (addr % 4096));
        }
        return (void*)addr;
    }
    return kmalloc(size);
}

void kfree(void* ptr) {
    if (!ptr) return;
    heap_block_t* block = (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
    if (block->magic != HEAP_MAGIC) return; // BozulmuÅŸ header!
    
    block->is_free = 1;
    
    // DayÄ± Tavsiyesi 4: Coalescing (BirleÅŸtirme)
    heap_block_t* curr = heap_start;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += curr->next->size + sizeof(heap_block_t);
            curr->next = curr->next->next;
            continue; // Bir daha kontrol et (Ã¼Ã§lÃ¼ birleÅŸme iÃ§in)
        }
        curr = curr->next;
    }
}

// --- SHM (Shared Memory) ---
#include "task.h"

typedef struct {
    uint32_t key;
    void* physical_frame;
    uint32_t size;
    int in_use;
} shm_region_t;

static shm_region_t shm_table[32];

int shm_get(uint32_t key, uint32_t size) {
    if (size > PAGE_SIZE) return -1; // Åimdilik tek sayfa
    
    // Var olanÄ± bul
    for (int i = 0; i < 32; i++) {
        if (shm_table[i].in_use && shm_table[i].key == key) {
            return i;
        }
    }
    
    // Yeni oluÅŸtur
    for (int i = 0; i < 32; i++) {
        if (!shm_table[i].in_use) {
            shm_table[i].key = key;
            shm_table[i].physical_frame = pmm_alloc_frame();
            shm_table[i].size = size;
            shm_table[i].in_use = 1;
            
            // SÄ±fÄ±rla
            switch_page_directory(kernel_directory);
            memset(shm_table[i].physical_frame, 0, PAGE_SIZE);
            if (current_task && current_task->page_directory) 
                switch_page_directory((page_directory_t*)current_task->page_directory);

            return i;
        }
    }
    return -1;
}

void* shm_at(int shm_id) {
    if (shm_id < 0 || shm_id >= 32 || !shm_table[shm_id].in_use) return 0;
    if (!current_task || !current_task->page_directory) return 0;
    
    // 0x10000000 (256MB) gibi sabit bir sanal adreste mapleyelim
    uint32_t shm_virt = 0x10000000 + (shm_id * PAGE_SIZE);
    map_page_in_dir((page_directory_t*)current_task->page_directory, 
                    shm_table[shm_id].physical_frame, (void*)shm_virt, 0x7);
                    
    return (void*)shm_virt;
}

void init_memory(uint32_t mem_size) {
    (void)mem_size; // Otomatik algÄ±la veya sabit kullan
    init_paging();
}
