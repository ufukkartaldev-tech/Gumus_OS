// GümüşOS Memory Management Unit Test
// Bellek yönetimi fonksiyonlarını test eder

#include <stddef.h>

// Bellek yönetimi sabitleri
#define MEMORY_SIZE 0x1000000    // 16MB
#define PAGE_SIZE 4096
#define PAGE_TABLE_SIZE 1024

// Test sonuçları
typedef enum {
    TEST_PASS,
    TEST_FAIL
} test_result_t;

// Bellek yönetimi yapıları
typedef struct {
    void* base;
    size_t size;
    size_t used;
} memory_heap_t;

typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} page_entry_t;

// Global heap
static memory_heap_t g_heap = { (void*)0x100000, MEMORY_SIZE, 0 };

// Test mesajları
static const char* MSG_START = "Memory Management Test Suite Baslatildi...";
static const char* MSG_MALLOC = "Malloc/Free Testi: ";
static const char* MSG_PAGE = "Paging Testi: ";
static const char* MSG_ALIGN = "Memory Alignment Testi: ";
static const char* MSG_BOUNDS = "Memory Bounds Testi: ";
static const char* MSG_PASS = "PASS";
static const char* MSG_FAIL = "FAIL";
static const char* MSG_DONE = "Memory Management Testleri Tamamlandi!";

// VGA yardımcı fonksiyonları
static void vga_putchar(char c, int x, int y, char color) {
    char* vga = (char*)0xB8000;
    vga[y * 80 * 2 + x * 2] = c;
    vga[y * 80 * 2 + x * 2 + 1] = color;
}

static void vga_print(const char* str, int x, int y, char color) {
    for (int i = 0; str[i]; i++) {
        vga_putchar(str[i], x + i, y, color);
    }
}

static void vga_clear_screen() {
    char* vga = (char*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x0F;
    }
}

// Basit malloc implementasyonu
static void* simple_malloc(size_t size) {
    if (g_heap.used + size > g_heap.size) {
        return NULL;
    }
    
    void* ptr = (char*)g_heap.base + g_heap.used;
    g_heap.used += size;
    return ptr;
}

static void simple_free(void* ptr) {
    // Basit implementasyon - gerçek OS'ta daha karmaşık olur
    // Şimdilik hiçbir şey yapmıyoruz
    (void)ptr;
}

// Page alignment fonksiyonu
static void* align_to_page(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (void*)((addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
}

// Page table oluşturma
static int create_page_table(page_entry_t* table, uint32_t base_frame) {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        table[i].present = 1;
        table[i].rw = 1;
        table[i].user = 0;
        table[i].accessed = 0;
        table[i].dirty = 0;
        table[i].unused = 0;
        table[i].frame = base_frame + i;
    }
    return 0;
}

// Test fonksiyonları
static test_result_t test_malloc_free() {
    vga_print(MSG_MALLOC, 0, 0, 0x0F);
    
    // Malloc testi
    void* ptr1 = simple_malloc(100);
    void* ptr2 = simple_malloc(200);
    void* ptr3 = simple_malloc(50);
    
    if (!ptr1 || !ptr2 || !ptr3) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Pointer'ların farklı olduğunu kontrol et
    if (ptr1 == ptr2 || ptr2 == ptr3 || ptr1 == ptr3) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Free testi
    simple_free(ptr1);
    simple_free(ptr2);
    simple_free(ptr3);
    
    vga_print(MSG_PASS, 50, 0, 0x0A);
    return TEST_PASS;
}

static test_result_t test_paging() {
    vga_print(MSG_PAGE, 0, 1, 0x0F);
    
    // Page table oluştur
    page_entry_t* page_table = (page_entry_t*)simple_malloc(sizeof(page_entry_t) * PAGE_TABLE_SIZE);
    if (!page_table) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    // Page table'ı doldur
    int result = create_page_table(page_table, 0x100);
    if (result != 0) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    // Page table içeriğini kontrol et
    if (page_table[0].present != 1 || page_table[0].frame != 0x100) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 1, 0x0A);
    return TEST_PASS;
}

static test_result_t test_memory_alignment() {
    vga_print(MSG_ALIGN, 0, 2, 0x0F);
    
    // Page alignment testi
    void* unaligned = (void*)0x1234;
    void* aligned = align_to_page(unaligned);
    
    uintptr_t aligned_addr = (uintptr_t)aligned;
    if (aligned_addr % PAGE_SIZE != 0) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // Alignment'in doğru olduğunu kontrol et
    if (aligned < unaligned) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 2, 0x0A);
    return TEST_PASS;
}

static test_result_t test_memory_bounds() {
    vga_print(MSG_BOUNDS, 0, 3, 0x0F);
    
    // Heap sınırlarını test et
    size_t initial_used = g_heap.used;
    
    // Normal malloc
    void* normal_ptr = simple_malloc(1000);
    if (!normal_ptr) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Heap dolana kadar malloc
    void* last_ptr = simple_malloc(g_heap.size - g_heap.used - 100);
    if (!last_ptr) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Fazla malloc denemesi (başarısız olmalı)
    void* overflow_ptr = simple_malloc(1000);
    if (overflow_ptr != NULL) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 3, 0x0A);
    return TEST_PASS;
}

// Ana test fonksiyonu
void kernel_main() {
    vga_clear_screen();
    vga_print(MSG_START, 0, 0, 0x0F);
    
    // Malloc/Free testleri
    test_result_t result = test_malloc_free();
    
    // Paging testleri
    result = test_paging();
    
    // Memory alignment testleri
    result = test_memory_alignment();
    
    // Memory bounds testleri
    result = test_memory_bounds();
    
    // Test sonu
    vga_print(MSG_DONE, 0, 5, 0x0F);
    
    // Sonsuz döngü
    while (1) {
        __asm__ volatile("hlt");
    }
}
