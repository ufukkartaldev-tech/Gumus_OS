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

void syscall_handler(registers_t* r) {
    uint32_t ret = 0;
    
    switch (r->eax) {
        case SYS_WRITE:
            // ebx: fd, ecx: buffer, edx: size
            if (r->ebx == 1) { // stdout
                char tmp[2] = {0,0};
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
            ret = vfs_read(r->ebx, (void*)r->ecx, r->edx);
            break;
        case SYS_OPEN:
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
            print("\nProcess Terminated.");
            // Görev sonlandırma mantığı eklenebilir
            while(1);
            break;
        default:
            print("\nUnknown Syscall");
            break;
    }
    
    r->eax = ret;
}
