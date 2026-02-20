#include "kernel.h"
#include "vfs.h"
#include "vga_gfx.h"

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

#define SYS_PIXEL 10
#define SYS_RECT  11
#define SYS_CLEAR 12
#define SYS_PRESENT 13

void syscall_handler(registers_t* r) {
    uint32_t ret = 0;
    
    switch (r->eax) {
        case SYS_WRITE:
            ret = vfs_write(r->ebx, (void*)r->ecx, r->edx);
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
            // ebx: x, ecx: y, edx: color
            vga_putpixel(r->ebx, r->ecx, (uint8_t)r->edx);
            break;
        case SYS_RECT:
            // esi: x, edi: y, ebx: w, ecx: h, edx: color
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
            while(1);
            break;
        default:
            print("\nUnknown Syscall");
            break;
    }
    
    r->eax = ret;
}
