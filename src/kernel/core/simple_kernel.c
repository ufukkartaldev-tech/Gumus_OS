#include "kernel.h"
#include "idt.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "gdt.h"

void kernel_main() {
    init_gdt();
    
    // Basit VGA test
    char* vga = (char*)0xB8000;
    for(int i = 0; i < 80*25; i++) {
        vga[i*2] = ' ';
        vga[i*2+1] = 0x0F; // Beyaz
    }
    
    // Mesaj yaz
    char* msg = "GumusOS Kernel Baslatildi!";
    int len = 0;
    while(msg[len]) len++;
    
    for(int i = 0; i < len; i++) {
        vga[i*2] = msg[i];
        vga[i*2+1] = 0x0A; // Yesil
    }
    
    __asm__ volatile("sti");
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
