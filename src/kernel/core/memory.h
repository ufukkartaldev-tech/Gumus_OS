#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096

// Bellek Haritası
#define KERNEL_START_PADDR  0x00100000 // 1MB
#define KERNEL_HEAP_START   0xC0000000 // 3GB Sanal Adres (Higher Half)
#define MEMORY_BITMAP_START 0x00200000 // 2MB Fiziksel

// Paging Yapıları
typedef struct {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t w_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;
    uint32_t page_size : 1; // 0 for 4KB, 1 for 4MB
    uint32_t global : 1;
    uint32_t avail : 3;
    uint32_t frame : 20; // Physical Address >> 12
} page_entry_t;

typedef struct {
    page_entry_t pages[1024];
} page_table_t;

typedef struct {
    page_table_t* tables[1024]; // Sanal işaretçiler
    uint32_t tablesPhysical[1024]; // CR3 için fiziksel adresler
} page_directory_t;

// Public Functions
void init_memory(uint32_t mem_size);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, uint32_t align);
void kfree(void* ptr);

// PMM Functions
void* pmm_alloc_frame();
void pmm_free_frame(void* frame);

// Paging Functions
void init_paging();
void map_page(void* phys, void* virt, uint32_t flags);
void* get_phys_addr(void* virt);
extern page_directory_t* kernel_directory;
void map_page_in_dir(page_directory_t* dir, void* phys, void* virt, uint32_t flags);
page_directory_t* create_process_directory();
void switch_page_directory(page_directory_t* dir);

#endif
