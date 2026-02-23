#include "kernel.h"
#include "idt.h"
#include "io.h"
#include "keyboard.h"
#include "utf8.h"
#include "memory.h"
#include "shell.h"
#include "vga_gfx.h"
#include "vga_font.h"
#include "string.h"
#include "cmos.h"
#include "driver.h"
#include "vfs.h"
#include "gdt.h"
#include "usb_host.h"
#include "mouse.h"
#include "task.h"
#include "window.h"
#include "file_manager.h"
#include "ata.h"
#include "serial.h"

// Function declarations
void draw_wallpaper();
void vga_draw_rect_transparent(int x, int y, int width, int height, uint8_t color);
void handle_beep_timer();
extern uint8_t* backbuffer;
void* get_vram();

void panic(const char* message, registers_t* r) {
    asm volatile("cli"); // Interruptları durdur

    // Ekranı temizle ve kırmızı bir panel çiz
    vga_clear(RED);
    draw_window(20, 40, 280, 120, " GUMUS OS: SISTEM DURDURULDU ", LIGHT_RED);
    
    int cursor_x = 30;
    int cursor_y = 65;
    print_color("Hata: ", YELLOW);
    print_color(message, WHITE);
    
    if (r) {
        cursor_x = 30;
        cursor_y = 85;
        print_color("Register Dump:", LIGHT_GREY);
        
        char buf[32];
        cursor_y += 10; cursor_x = 30;
        print("EAX: "); itoa(r->eax, buf); print(buf); print(" EBX: "); itoa(r->ebx, buf); print(buf);
        cursor_y += 8; cursor_x = 30;
        print("ECX: "); itoa(r->ecx, buf); print(buf); print(" EDX: "); itoa(r->edx, buf); print(buf);
        cursor_y += 8; cursor_x = 30;
        print("EIP: "); itoa(r->eip, buf); print(buf); print(" CS:  "); itoa(r->cs, buf); print(buf);
        cursor_y += 8; cursor_x = 30;
        print("ESP: "); itoa(r->esp, buf); print(buf); print(" EFLAGS: "); itoa(r->eflags, buf); print(buf);
    }

    cursor_y = 145; cursor_x = 30;
    print_color("Sistem guvenli modda durduruldu.", LIGHT_CYAN);
    print_color("\nLutfen bilgisayari resetleyin.", LIGHT_CYAN);

    vga_present();
    while(1) { asm volatile("hlt"); }
}

#include "vga_font.h"

static utf8_decoder_t decoder = {0, 0};

// Global ekran pozisyonu (typing iÃ§in) - Unused in graphics mode
// static int cursor_x = 0;
// static int cursor_y = 1; 
static uint8_t current_color = 0x0F;

// Grafik modu cursor takibi (piksel cinsinden)
static int gfx_cursor_x = 0;
static int gfx_cursor_y = 160; // Splash logosunun altÄ±nda baÅŸlasÄ±n

// Grafik modunda donanÄ±m imleci kullanÄ±lmaz
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    (void)cursor_start; (void)cursor_end;
}

void update_cursor(int x, int y) {
    // Grafik modunda imleÃ§, Ã§izim fonksiyonlarÄ± ile yÃ¶netilir
    // Ä°leride buraya yazÄ±lÄ±msal imleÃ§ (yanÄ±p sÃ¶nen kare vb.) eklenebilir
    (void)x; (void)y;
}

static int cursor_blink_state = 0; // 0: Silik, 1: GÃ¶rÃ¼nÃ¼r

// Keyboard Buffer for Syscalls
#define KBD_BUF_SIZE 128
static char kbd_buffer[KBD_BUF_SIZE];
static int kbd_head = 0;
static int kbd_tail = 0;

void kbd_put(char c) {
    int next = (kbd_head + 1) % KBD_BUF_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

char kbd_get() {
    if (kbd_head == kbd_tail) return 0;
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
    return c;
}

void draw_cursor(uint8_t color) {
    // 8x8 font iÃ§in alt kÄ±sma (7. satÄ±r) tire Ã§iziyoruz
    // Ä°mleÃ§ boyutu: 8x2 piksel
    if (gfx_cursor_y >= SCREEN_HEIGHT) return;
    vga_draw_rect(gfx_cursor_x, gfx_cursor_y + 6, 8, 2, color);
}

void scroll() {
    if (backbuffer) {
        memcpy(backbuffer, backbuffer + SCREEN_WIDTH * 8, SCREEN_WIDTH * (SCREEN_HEIGHT - 8));
        memset(backbuffer + SCREEN_WIDTH * (SCREEN_HEIGHT - 8), 0, SCREEN_WIDTH * 8);
    } else {
        memcpy(get_vram(), get_vram() + SCREEN_WIDTH * 8, SCREEN_WIDTH * (SCREEN_HEIGHT - 8));
        memset(get_vram() + SCREEN_WIDTH * (SCREEN_HEIGHT - 8), 0, SCREEN_WIDTH * 8);
    }
    
    gfx_cursor_y -= 8;
    if (gfx_cursor_y < 0) gfx_cursor_y = 0;
}

void putchar(char c) {
    // Yazmadan Ã¶nce mevcut imleci sil (Siyah)
    draw_cursor(0);
    
    // Grafik Modu Text BasÄ±mÄ± (320x200 iÃ§in)
    if (c == '\n') {
        gfx_cursor_x = 0;
        gfx_cursor_y += 8;
    } else if (c == '\b') {
        if (gfx_cursor_x >= 8) gfx_cursor_x -= 8;
        vga_draw_rect(gfx_cursor_x, gfx_cursor_y, 8, 8, 0); // Siyahla sil
    } else {
        vga_draw_char(gfx_cursor_x, gfx_cursor_y, c, 15); // Beyaz harf
        gfx_cursor_x += 8;
    }

    // Ekran sonu kontrolÃ¼
    if (gfx_cursor_x >= SCREEN_WIDTH) {
        gfx_cursor_x = 0;
        gfx_cursor_y += 8;
    }
    if (gfx_cursor_y >= SCREEN_HEIGHT - 8) {
        scroll();
    }
    
    // Ä°ÅŸlem bitince yeni pozisyona imleci Ã§iz (Beyaz)
    draw_cursor(15);
    cursor_blink_state = 1; // GÃ¶rÃ¼nÃ¼r yap ve zamanlamayÄ± bekle
}

void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

void print_color(const char* str, uint8_t color) {
    uint8_t old_color = current_color;
    current_color = color;
    print(str);
    current_color = old_color;
}

void update_status_bar() {
    // Grafik Modu Status Bar
    // Ãœstte 10 piksellik mavi ÅŸerit
    vga_draw_rect(0, 0, SCREEN_WIDTH, 10, 1); // 1 = Mavi
    
    vga_draw_text(5, 1, "GumusOS: Uyanis", 15); // Beyaz
    vga_draw_text(SCREEN_WIDTH - 60, 1, "v0.1.0", 15);
}

void draw_logo() {
    print_color("\n", WHITE);
    print_color("   ______                               ____  _____\n", LIGHT_CYAN);
    print_color("  / ____/_  ______ ___  __  _______ ____  / ___/ \n", LIGHT_CYAN);
    print_color(" / / __/ / / / __ `__ \\/ / / / ___/ __ \\/ /__  \n", LIGHT_CYAN);
    print_color("/ /_/ / /_/ / / / / / / /_/ (__  ) /_/ / /___/  \n", LIGHT_CYAN);
    print_color("\\____/\\__,_/_/ /_/ /_/\\__,_/____/\\____/\\____/   \n", LIGHT_CYAN);
    print_color("\n         GUMUSHANE'DEN DUNYAYA 'UYANIS'\n\n", YELLOW);
}

void draw_window(int x, int y, int w, int h, const char* title, uint8_t color) {
    // Text mode coordinates -> Pixel coordinates approximation
    // 320x200 resolution means 40x25 text (8x8 font)
    int px = x * 8;
    int py = y * 8;
    int pw = w * 8;
    int ph = h * 8;

    // Background (Clear content)
    vga_draw_rect(px, py, pw, ph, 0); 

    // Borders
    vga_draw_rect(px, py, pw, 1, color); // Top
    vga_draw_rect(px, py + ph - 1, pw, 1, color); // Bottom
    vga_draw_rect(px, py, 1, ph, color); // Left
    vga_draw_rect(px + pw - 1, py, 1, ph, color); // Right

    // Title
    if (title) {
        int title_len = strlen(title);
        // Center title
        int title_px = px + (pw - (title_len * 8)) / 2;
        if (title_px < px) title_px = px; // Clip check
        vga_draw_text(title_px, py + 2, title, 15); // White
    }
}

void update_clock() {
    uint8_t sec, min, hour;
    get_rtc_time(&sec, &min, &hour);

    char time_str[9];
    time_str[0] = (hour / 10) + '0';
    time_str[1] = (hour % 10) + '0';
    time_str[2] = ':';
    time_str[3] = (min / 10) + '0';
    time_str[4] = (min % 10) + '0';
    time_str[5] = ':';
    time_str[6] = (sec / 10) + '0';
    time_str[7] = (sec % 10) + '0';
    time_str[8] = '\0';

    // SaÄŸ alt kÃ¶ÅŸeye (Taskbar iÃ§ine) saat Ã§iz
    // Taskbar yÃ¼kseklik: 20 (y=SCREEN_HEIGHT-20)
    int clock_x = SCREEN_WIDTH - 60;
    int clock_y = SCREEN_HEIGHT - 14;
    vga_draw_rect(clock_x, clock_y, 50, 10, 7); // Arka plan
    vga_draw_text(clock_x + 3, clock_y + 1, time_str, 0); // Siyah yazÄ±
}

static uint32_t timer_ticks = 0;

uint32_t get_timer_ticks() {
    return timer_ticks;
}

// Timer kesmesi iÅŸleyicisi (Multitasking scheduler tarafÄ±ndan da Ã§aÄŸrÄ±labilir)
void timer_handler() {
    timer_ticks++;
    if (timer_ticks % 18 == 0) { // YaklaÅŸÄ±k saniyede bir
        update_clock();
    }
    
    // Pencereleri GÃ¼ncelle (Oyun DÃ¶ngÃ¼sÃ¼)
    update_windows(); // Logic updates
    
    // --- RENDER LOOP (Double Buffering) ---
    // Her tick'te ekranÄ± yeniden Ã§iziyoruz (yaklaÅŸÄ±k 18 FPS)
    // 1. Arka planÄ± ve masaÃ¼stÃ¼nÃ¼ Ã§iz (clear_screen fonksiyonu bu iÅŸi yapÄ±yor)
    // clear_screen(); 
    // FAKAT clear_screen iÃ§inde draw_windows var.
    // Her ÅŸeyi sÄ±rayla Ã§izelim:
    
    // A) Arkaplan (Wallpaper)
    // Simple gradient background
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        vga_draw_line(0, i, SCREEN_WIDTH - 1, i, (i * 2) % 256);
    }
    
    // B) MasaÃ¼stÃ¼ Ä°konlarÄ±
    // ... (Aynen kalsÄ±n)
    
    // C) GÃ¶rev Ã‡ubuÄŸu ve Saat (Åeffaf!)
    update_status_bar(); // Ãœst bar
    
    // Taskbar (Normal draw_taskbar yerine custom ÅŸeffaf Ã§izim yapalÄ±m veya onu modifiye edelim)
    // Kernel.c iÃ§inde draw_taskbar yok, extern de deÄŸil (muhtemelen shell.h veya window.h iÃ§inde deÄŸil, kernel.c'de aÅŸaÄŸÄ±da tanÄ±mlÄ± olabilir veya eksik).
    // Aaa, kernel.c iÃ§inde draw_taskbar() Ã§aÄŸÄ±rÄ±lÄ±yor ama tanÄ±mÄ± bu dosyanÄ±n altÄ±ndaysa gÃ¶remeyebilirim.
    // draw_taskbar window.c iÃ§inde olabilir mi?
    // Bir dakika, Ã¶nceki view_file Ã§Ä±ktÄ±sÄ±nda draw_taskbar() Ã§aÄŸrÄ±sÄ± var (satÄ±r 269) ama tanÄ±mÄ± GÃ–RÃœNMÃœYOR.
    // Muhtemelen kernel.c'nin alt kÄ±sÄ±mlarÄ±nda veya bu dosya iÃ§inde gizli?
    // HayÄ±r, view_file tÃ¼m dosyayÄ± gÃ¶sterdi. draw_taskbar yok!
    // Demek ki Implicit Declaration warning alÄ±yoruz ve link hatasÄ± vermiyor Ã§Ã¼nkÃ¼ baÅŸka bir yerde tanÄ±mlÄ±.
    // window.c'ye bakalÄ±m.
    
    // Şimdilik manuel çizelim (Şeffaf Taskbar):
    // Simple transparent rectangle (alpha blending simulation)
    for(int y = SCREEN_HEIGHT - 20; y < SCREEN_HEIGHT; y++) {
        for(int x = 0; x < SCREEN_WIDTH; x++) {
            uint8_t bg_color = (y * 2) % 256; // Background color
            uint8_t fg_color = 1; // Blue
            // Simple alpha blending (50%)
            uint8_t final_color = (bg_color + fg_color) / 2;
            vga_putpixel(x, y, final_color);
        }
    }
    vga_draw_line(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 20, 15); 
    
    int btn_y = SCREEN_HEIGHT - 18;
    vga_draw_rect(2, btn_y, 50, 16, 7); // Gri
    vga_draw_rect(2, btn_y, 50, 1, 15); // Highlight
    vga_draw_rect(2, btn_y, 1, 16, 15);
    vga_draw_rect(51, btn_y, 1, 16, 0); // Shadow
    vga_draw_rect(2, btn_y + 15, 50, 1, 0);
    vga_draw_text(10, btn_y + 4, "GUMUS", 0);
    
    update_clock(); // Saati Ã§iz
    
    // D) Pencereler
    draw_windows();
    
    // E) Fare
    draw_mouse_cursor();
    
    // F) Ekrana Bas!
    vga_present();
    
    // --------------------------------------

    // Simple beep timer handler (placeholder)
    // handle_beep_timer(); // Ses süresini yönet

    // Ä°mleÃ§ YanÄ±p SÃ¶nme Efekti (YaklaÅŸÄ±k 0.5 saniyede bir)
    /*
    if (timer_ticks % 9 == 0) {
        if (cursor_blink_state) draw_cursor(0); // Sil
        else draw_cursor(15); // Ã‡iz
        cursor_blink_state = !cursor_blink_state;
    }
    */
    // Double buffering varken cursor_blink farklÄ± yÃ¶netilmeli (her frame Ã§izilmeli)
    // Åimdilik disable edelim veya sadece timer'a baÄŸlÄ± Ã§izelim.
    // draw_cursor() fonksiyonu backbuffer'a Ã§izer.
}

void user_test_app() {
    const char* msg = "\n[USER] Merhaba Ring 3! Syscall basarili.\n";
    // SYS_WRITE (4), fd=1, buf=msg, count=40
    asm volatile("mov $4, %%eax; mov $1, %%ebx; mov %0, %%ecx; mov $40, %%edx; int $0x80" : : "r"(msg) : "eax", "ebx", "ecx", "edx");
    while(1) {
        // DÃ¶ngÃ¼de kal, multitasking diÄŸer tasklara geÃ§ecek
    }
}

void isr_handler(struct registers* r) {
    (void)r;
    print("\n[!] KRITIK HATA: SISTEM DURDURULDU.");
    while(1) hlt();
}

void irq_handler(struct registers* r) {
    if (r->int_no >= 40) outb(0xA0, 0x20); // Slave
    outb(0x20, 0x20); // Master

    if (r->int_no == 32) { // Timer
        timer_handler();
        schedule((uint32_t)r);
    } else if (r->int_no == 33) {
        uint8_t scancode = inb(0x60);
        
        if (!(scancode & 0x80)) {
            handle_keyboard(scancode);
        }
    } else if (r->int_no == 44) { // IRQ12 - Fare
        handle_mouse_packet();
    }
}

void clear_screen() {
    // Shell iÃ§in: Sadece arkaplanÄ± temizle (backbuffer'a)
    // 1. Degrade Mavi Arkaplan (Temel)
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        vga_draw_line(0, i, SCREEN_WIDTH - 1, i, 104);
    }
    
    gfx_cursor_x = 0;
    gfx_cursor_y = 12; // Status bar'Ä±n hemen altÄ±
}

#include "sound.h"

void msleep(uint32_t ms) {
    uint32_t start = get_timer_ticks();
    uint32_t ticks = ms / 55; // 18.2 ticks per second approx
    if (ticks == 0) ticks = 1;
    while (get_timer_ticks() - start < ticks) {
        hlt();
    }
}

void beep_hz(uint32_t hz, uint32_t ms) {
    play_sound(hz);
    msleep(ms);
    nosound();
}

void kernel_main() {
    init_gdt();
    init_memory(0); 
    
    vga_init_double_buffer(); // Double Buffering BaÅŸlat
    vga_clear(0); // Siyah ekran
    
    // Arka plana renk katalÄ±m
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        vga_draw_line(0, i, SCREEN_WIDTH - 1, i, i % 256);
    }
    
    vga_draw_rect(SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 - 50, 220, 100, 0); // Siyah kutu
    vga_draw_text(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 30, "GUMUS OS UYANIS", 14); // SarÄ± yazÄ±
    vga_draw_text(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, "VBE 800x600 AKTIF!", 10); // YeÅŸil yazÄ±
    vga_draw_rect(SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 + 48, 220, 2, 15); // Alt Ã§izgi

    init_idt();
    
    // Sistem AÃ§Ä±lÄ±ÅŸ Sesi
    beep_hz(440, 100);
    beep_hz(880, 100);
    
    // ModÃ¼ler SÃ¼rÃ¼cÃ¼ Sistemi BaÅŸlatÄ±lÄ±yor
    driver_manager_init();
    pseudo_drivers_init(); // Null, Zero sÃ¼rÃ¼cÃ¼lerini yÃ¼kle
    hardware_detect_init(); // DonanÄ±m tespiti ve sÃ¼rÃ¼cÃ¼ yÃ¼kleme
    
    ata_init_driver();
    driver_register(create_serial_driver()); // Seri Port SÃ¼rÃ¼cÃ¼sÃ¼nÃ¼ Kaydet
    vfs_init();
    
    // USB Host Controller Sistemi BaÅŸlatÄ±lÄ±yor
    usb_host_init();
    
    mouse_init(); // Fareyi BaÅŸlat
    
    init_multitasking(); // Ã‡oklu GÃ¶rev HazÄ±rlÄ±ÄŸÄ± (PID 0 Kernel KaydÄ±)
    create_user_process(user_test_app); // Yeni: KullanÄ±cÄ± Modu SÃ¼reci BaÅŸlat
    
    init_window_manager(); // Pencere YÃ¶neticisi BaÅŸlat
    init_file_manager();   // Dosya YÃ¶neticisi BaÅŸlat

    __asm__ volatile("sti");
    shell_init();

    while(1) {
        hlt();
    }
}
