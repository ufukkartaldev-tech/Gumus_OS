void __stdcall kernel_main() {
    // En basit test - sadece bir karakter
    char* vga = (char*)0xB8000;
    vga[0] = 'A';
    vga[1] = 0x0F; // Beyaz
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
