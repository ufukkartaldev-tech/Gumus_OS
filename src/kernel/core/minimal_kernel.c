#include "kernel.h"

void kernel_main() {
    // 32-bit Protected Mode test
    char* vga = (char*)0xB8000;
    
    // Ekranı temizle
    for(int i = 0; i < 80*25; i++) {
        vga[i*2] = ' ';
        vga[i*2+1] = 0x0F; // Beyaz
    }
    
    // Mesaj yaz
    char* msg = "GumusOS 32-bit Protected Mode Aktif!";
    int i = 0;
    while(msg[i]) {
        vga[i*2] = msg[i];
        vga[i*2+1] = 0x0A; // Yesil
        i++;
    }
    
    // Sonsuz döngü
    while(1) {
        __asm__ volatile("hlt");
    }
}
