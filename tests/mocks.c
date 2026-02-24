#include <stdint.h>
#include <stddef.h>
#include "../src/kernel/cpu/task.h"
#include "../src/kernel/core/string.h"

// --- Global Mock Data ---
static uint8_t mock_heap[1024 * 64]; // 64KB Mock Heap
static uint32_t mock_heap_ptr = 0;

static char mock_file_buffer[] = "GUMUSOS_VER=2.0\nDEBUG=TRUE\n";
static task_t mock_current_task = {0, 1, TASK_RUNNING, 0, 0, 0, 0, 0, 0};
task_t* current_task = &mock_current_task;

// --- System Mocks ---
void putchar(char c) {}
void print(const char* s) {}
void print_color(const char* s, uint8_t c) {}
void vga_present() {}
void vga_clear(uint8_t c) {}
void vga_putpixel(int x, int y, uint8_t c) {}
void vga_draw_rect(int x, int y, int w, int h, uint8_t c) {}
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t c) {}
void vga_draw_char(int x, int y, char c, uint8_t clr) {}
void vga_draw_text(int x, int y, const char* s, uint8_t c) {}
void vga_init_double_buffer() {}
void update_windows() {}
void draw_windows() {}
void draw_mouse_cursor() {}
void draw_taskbar() {}
void outb(uint16_t p, uint8_t d) {}
uint8_t inb(uint16_t p) { return 0; }
void outw(uint16_t p, uint16_t d) {}
uint16_t inw(uint16_t p) { return 0; }

void cli() { __asm__ volatile("cli"); }
void sti() { __asm__ volatile("sti"); }
void hlt() { __asm__ volatile("hlt"); }

void get_rtc_time(uint8_t* s, uint8_t* m, uint8_t* h) {
    if(s) *s = 0; if(m) *m = 0; if(h) *h = 0;
}

void init_gdt() {}
void init_idt() {}
void schedule(uint32_t e) {}
void driver_manager_init() {}
void hardware_detect_init() {}
void pseudo_drivers_init() {}
void ata_init_driver() {}
void driver_register(void* d) {}
void usb_host_init() {}
void mouse_init() {}
void vfs_init() {}
void init_window_manager() {}
void init_file_manager() {}
void shell_init() {}
void handle_keyboard(uint8_t s) {}
void handle_mouse_packet() {}
char kbd_get() { return 0; }
void kbd_put(char c) {}
void play_sound(uint32_t f) {}
void nosound() {}
void update_status_bar() {}
void draw_logo() {}
void update_clock() {}
void panic(const char* m, void* r) { while(1); }

// --- VFS Mocks (Dayı Tavsiyesi 1) ---
int vfs_open(const char* path, int flags) {
    if (strcmp(path, "/etc/config") == 0) return 100; // Success handle
    return -1;
}

void vfs_close(int fd) {}

int vfs_read(int fd, void* buf, size_t count) {
    if (fd == 100) {
        size_t len = strlen(mock_file_buffer);
        if (count > len) count = len;
        memcpy(buf, mock_file_buffer, count);
        return (int)count;
    }
    return 0;
}

int vfs_write(int fd, const void* buf, size_t count) { return (int)count; }

// --- Memory Mocks (Dayı Tavsiyesi 2) ---
void map_page_in_dir(page_directory_t* dir, void* phys, void* virt, uint32_t flags) {}
void* kmalloc_aligned(size_t size, uint32_t align) {
    // Basic bump allocation on mock_heap
    if (mock_heap_ptr % align != 0) {
        mock_heap_ptr += (align - (mock_heap_ptr % align));
    }
    void* res = &mock_heap[mock_heap_ptr];
    mock_heap_ptr += size;
    return res;
}

void* shm_at(int shm_id) { return 0; }
int shm_get(uint32_t key, uint32_t size) { return -1; }
void task_exit(int code) {}
int task_kill(uint32_t pid) { return -1; }
void tss_set_stack(uint16_t ss, uint32_t esp) {}

// --- Memory Köprüsü Mocks (Dayı Tavsiyesi 3) ---
void copy_to_user(void* dest, const void* src, size_t size) {
    memcpy(dest, src, size);
}

void copy_from_user(void* dest, const void* src, size_t size) {
    memcpy(dest, src, size);
}
