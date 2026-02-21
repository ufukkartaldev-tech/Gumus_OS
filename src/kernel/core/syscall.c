#include "kernel.h"
#include "vfs.h"
#include "vga_gfx.h"
#include "task.h"
#include "memory.h"

typedef struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

// Syscall Numaraları
#define SYS_EXIT  1
#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6
#define SYS_SBRK  7

#define SYS_PIXEL 10
#define SYS_RECT  11
#define SYS_CLEAR 12
#define SYS_PRESENT 13

// Kullanıcıdan gelen pointer'ın güvenli (User space'de) olup olmadığını kontrol et
static int validate_user_ptr(void* ptr, uint32_t size) {
    uint32_t addr = (uint32_t)ptr;
    // GümüşOS'ta Higher Half çekirdek 0xC0000000 (3GB) adresinden başlar.
    // İlk 16MB ise identity map (Kernel) alanıdır.
    // Kullanıcı alanı şimdilik 0x400000 (4MB) ile 0xC0000000 arasındadır dersek:
    if (addr < 0x400000 || addr >= 0xC0000000) return 0;
    if (addr + size > 0xC0000000) return 0;
    return 1;
}

uint32_t syscall_handler(registers_t* r) {
    uint32_t ret = 0;
    
    switch (r->eax) {
        case SYS_WRITE:
            // ebx: fd, ecx: buffer, edx: size
            if (!validate_user_ptr((void*)r->ecx, r->edx)) {
                print_color("\n[SYSCALL] Guvenlik Ihlali: Gecersiz pointer!", LIGHT_RED);
                ret = -1;
                break;
            }
            
            if (r->ebx == 1) { // stdout
                char* buf = (char*)r->ecx;
                for(uint32_t i=0; i<r->edx; i++) {
                    putchar(buf[i]);
                }
                ret = r->edx;
            } else {
                ret = vfs_write(r->ebx, (void*)r->ecx, r->edx);
            }
            break;
        case SYS_SBRK:
            // ebx: increment (bytes)
            if (current_task) {
                uint32_t old_break = current_task->mem_break;
                uint32_t new_break = old_break + r->ebx;
                
                // Gerekli sayfaları map et
                for (uint32_t v = (old_break + 4095) & 0xFFFFF000; v < new_break; v += PAGE_SIZE) {
                    void* frame = pmm_alloc_frame();
                    map_page_in_dir((page_directory_t*)current_task->page_directory, frame, (void*)v, 0x7);
                }
                
                current_task->mem_break = new_break;
                ret = old_break;
            }
            break;
        case SYS_READ:
            if (!validate_user_ptr((void*)r->ecx, r->edx)) return -1;
            ret = vfs_read(r->ebx, (void*)r->ecx, r->edx);
            break;
        case SYS_OPEN:
            // ebx: path
            if (!validate_user_ptr((void*)r->ebx, 1)) return -1; // En azından ilk karakter
            ret = vfs_open((char*)r->ebx, r->ecx);
            break;
        case SYS_CLOSE:
            vfs_close(r->ebx);
            break;
        case SYS_PIXEL:
            vga_putpixel(r->ebx, r->ecx, (uint8_t)r->edx);
            break;
        case SYS_RECT:
            vga_draw_rect(r->esi, r->edi, r->ebx, r->ecx, (uint8_t)r->edx);
            break;
        case SYS_CLEAR:
            vga_clear((uint8_t)r->ebx);
            break;
        case SYS_PRESENT:
            vga_present();
            break;
        case SYS_EXIT:
            task_exit(r->ebx); // ebx: exit_code
            return schedule((uint32_t)r); // Görev değiştir ve asla geri dönme
        default:
            print("\nUnknown Syscall");
            break;
    }
    
    r->eax = ret;
    return (uint32_t)r;
}
