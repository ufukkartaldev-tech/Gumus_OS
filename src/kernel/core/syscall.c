#include "kernel.h"
#include "vfs.h"
#include "vga_gfx.h"
#include "task.h"
#include "memory.h"
#include "idt.h"

// Syscall NumaralarÄ±
#define SYS_EXIT  1
#define SYS_READ  3
#define SYS_WRITE 4
#define SYS_OPEN  5
#define SYS_CLOSE 6
#define SYS_SBRK  7

#define SYS_PIXEL 20
#define SYS_RECT  21
#define SYS_CLEAR 22
#define SYS_PRESENT 23

// KullanÄ±cÄ±dan gelen pointer'Ä±n gÃ¼venli (User space'de) olup olmadÄ±ÄŸÄ±nÄ± kontrol et
static int validate_user_ptr(void* ptr, uint32_t size) {
    uint32_t addr = (uint32_t)ptr;
    // GÃ¼mÃ¼ÅŸOS'ta Higher Half Ã§ekirdek 0xC0000000 (3GB) adresinden baÅŸlar.
    // Ä°lk 16MB ise identity map (Kernel) alanÄ±dÄ±r.
    // KullanÄ±cÄ± alanÄ± ÅŸimdilik 0x400000 (4MB) ile 0xC0000000 arasÄ±ndadÄ±r dersek:
    if (addr < 0x400000 || addr >= 0xC0000000) return 0;
    if (addr + size > 0xC0000000) return 0;
    return 1;
}

// KullanÄ±cÄ±dan gelen string'in (NULL-terminated) gÃ¼venli olup olmadÄ±ÄŸÄ±nÄ± kontrol et
static int validate_user_str(const char* str) {
    if (!validate_user_ptr((void*)str, 1)) return 0;
    
    char* s = (char*)str;
    while (validate_user_ptr(s, 1)) {
        if (*s == '\0') return 1;
        s++;
    }
    return 0; // NULL gelmeden kullanÄ±cÄ± alanÄ± dÄ±ÅŸÄ±na Ã§Ä±ktÄ±
}

uint32_t syscall_handler(registers_t* r) {
    uint32_t ret = 0;
    
    switch (r->eax) {
        case SYS_WRITE:
            // ebx: fd, ecx: buffer, edx: size
            if (!validate_user_ptr((void*)r->ecx, r->edx)) {
                print_color("\n[SYSCALL] Guvenlik Ihlali: Gecersiz buffer pointer!", LIGHT_RED);
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
                
                // GÃ¼venlik: Ã‡ekirdek belleÄŸine (0xC0000000) girmesini engelle
                // AyrÄ±ca stack ile Ã§akÄ±ÅŸmadÄ±ÄŸÄ±ndan emin ol (User stack 0xBFFFF000 civarÄ± baÅŸlar)
                if (new_break >= 0xB0000000) {
                    print_color("\n[SYSCALL] Hata: Bellek siniri asildi (sbrk)!", LIGHT_RED);
                    ret = -1;
                    break;
                }
                
                // Gerekli sayfalarÄ± map et
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
            if (r->ebx == 0) { // stdin (Keyboard)
                char* buf = (char*)r->ecx;
                uint32_t count = 0;
                while (count < r->edx) {
                    char c = kbd_get();
                    if (c != 0) {
                        buf[count++] = c;
                        //putchar(c); // Echo is usually handled by app or shell
                        if (c == '\n') break;
                    } else {
                        asm volatile("hlt"); // Wait for interrupt
                    }
                }
                ret = count;
            } else {
                ret = vfs_read(r->ebx, (void*)r->ecx, r->edx);
            }
            break;
        case SYS_OPEN:
            // ebx: path
            if (!validate_user_str((char*)r->ebx)) {
                print_color("\n[SYSCALL] Guvenlik Ihlali: Gecersiz path pointer!", LIGHT_RED);
                ret = -1;
                break;
            }
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
            return schedule((uint32_t)r); // GÃ¶rev deÄŸiÅŸtir ve asla geri dÃ¶nme
        case SYS_KILL:
            // ebx: target_pid
            ret = task_kill(r->ebx);
            break;
        case SYS_SHM_GET:
            // ebx: key, ecx: size
            ret = shm_get(r->ebx, r->ecx);
            break;
        case SYS_SHM_AT:
            // ebx: shm_id
            ret = (uint32_t)shm_at(r->ebx);
            break;
        default:
            print("\nUnknown Syscall");
            break;
    }
    
    r->eax = ret;
    return (uint32_t)r;
}
