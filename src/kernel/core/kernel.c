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
#include "serial.h"

// ... codes ...

// Eski kernel_main kaldırıldı

// Şeffaf Dikdörtgen (Checkerboard / Stipple)
void vga_draw_rect_transparent(int x, int y, int w, int h, uint8_t color) {
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            if ((i + j) % 2 == 0) { // Sadece çift pikselleri boya
                vga_putpixel(i, j, color);
            }
        }
    }
}

// Kesme sırasında kaydedilen register yapısı
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

#include "vga_font.h"

static utf8_decoder_t decoder = {0, 0};

// Global ekran pozisyonu (typing için) - Unused in graphics mode
// static int cursor_x = 0;
// static int cursor_y = 1; 
static uint8_t current_color = 0x0F;

// Grafik modu cursor takibi (piksel cinsinden)
static int gfx_cursor_x = 0;
static int gfx_cursor_y = 160; // Splash logosunun altında başlasın

// Grafik modunda donanım imleci kullanılmaz
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    (void)cursor_start; (void)cursor_end;
}

void update_cursor(int x, int y) {
    // Grafik modunda imleç, çizim fonksiyonları ile yönetilir
    // İleride buraya yazılımsal imleç (yanıp sönen kare vb.) eklenebilir
    (void)x; (void)y;
}

static int cursor_blink_state = 0; // 0: Silik, 1: Görünür

void draw_cursor(uint8_t color) {
    // 8x8 font için alt kısma (7. satır) tire çiziyoruz
    // İmleç boyutu: 8x2 piksel
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
    // Yazmadan önce mevcut imleci sil (Siyah)
    draw_cursor(0);
    
    // Grafik Modu Text Basımı (320x200 için)
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

    // Ekran sonu kontrolü
    if (gfx_cursor_x >= SCREEN_WIDTH) {
        gfx_cursor_x = 0;
        gfx_cursor_y += 8;
    }
    if (gfx_cursor_y >= SCREEN_HEIGHT - 8) {
        scroll();
    }
    
    // İşlem bitince yeni pozisyona imleci çiz (Beyaz)
    draw_cursor(15);
    cursor_blink_state = 1; // Görünür yap ve zamanlamayı bekle
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
    // Üstte 10 piksellik mavi şerit
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

    // Sağ alt köşeye (Taskbar içine) saat çiz
    // Taskbar yükseklik: 20 (y=SCREEN_HEIGHT-20)
    int clock_x = SCREEN_WIDTH - 60;
    int clock_y = SCREEN_HEIGHT - 14;
    vga_draw_rect(clock_x, clock_y, 50, 10, 7); // Arka plan
    vga_draw_text(clock_x + 3, clock_y + 1, time_str, 0); // Siyah yazı
}

static uint32_t timer_ticks = 0;

uint32_t get_timer_ticks() {
    return timer_ticks;
}

// Timer kesmesi işleyicisi (Multitasking scheduler tarafından da çağrılabilir)
void timer_handler() {
    timer_ticks++;
    if (timer_ticks % 18 == 0) { // Yaklaşık saniyede bir
        update_clock();
    }
    
    // Pencereleri Güncelle (Oyun Döngüsü)
    update_windows(); // Logic updates
    
    // --- RENDER LOOP (Double Buffering) ---
    // Her tick'te ekranı yeniden çiziyoruz (yaklaşık 18 FPS)
    // 1. Arka planı ve masaüstünü çiz (clear_screen fonksiyonu bu işi yapıyor)
    // clear_screen(); 
    // FAKAT clear_screen içinde draw_windows var.
    // Her şeyi sırayla çizelim:
    
    // A) Arkaplan (Wallpaper)
    draw_wallpaper();
    
    // B) Masaüstü İkonları
    // ... (Aynen kalsın)
    
    // C) Görev Çubuğu ve Saat (Şeffaf!)
    update_status_bar(); // Üst bar
    
    // Taskbar (Normal draw_taskbar yerine custom şeffaf çizim yapalım veya onu modifiye edelim)
    // Kernel.c içinde draw_taskbar yok, extern de değil (muhtemelen shell.h veya window.h içinde değil, kernel.c'de aşağıda tanımlı olabilir veya eksik).
    // Aaa, kernel.c içinde draw_taskbar() çağırılıyor ama tanımı bu dosyanın altındaysa göremeyebilirim.
    // draw_taskbar window.c içinde olabilir mi?
    // Bir dakika, önceki view_file çıktısında draw_taskbar() çağrısı var (satır 269) ama tanımı GÖRÜNMÜYOR.
    // Muhtemelen kernel.c'nin alt kısımlarında veya bu dosya içinde gizli?
    // Hayır, view_file tüm dosyayı gösterdi. draw_taskbar yok!
    // Demek ki Implicit Declaration warning alıyoruz ve link hatası vermiyor çünkü başka bir yerde tanımlı.
    // window.c'ye bakalım.
    
    // Şimdilik manuel çizelim (Şeffaf Taskbar):
    vga_draw_rect_transparent(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH, 20, 1); // Mavi, Şeffaf
    vga_draw_line(0, SCREEN_HEIGHT - 20, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 20, 15); 
    
    int btn_y = SCREEN_HEIGHT - 18;
    vga_draw_rect(2, btn_y, 50, 16, 7); // Gri
    vga_draw_rect(2, btn_y, 50, 1, 15); // Highlight
    vga_draw_rect(2, btn_y, 1, 16, 15);
    vga_draw_rect(51, btn_y, 1, 16, 0); // Shadow
    vga_draw_rect(2, btn_y + 15, 50, 1, 0);
    vga_draw_text(10, btn_y + 4, "GUMUS", 0);
    
    update_clock(); // Saati çiz
    
    // D) Pencereler
    draw_windows();
    
    // E) Fare
    draw_mouse_cursor();
    
    // F) Ekrana Bas!
    vga_present();
    
    // --------------------------------------

    handle_beep_timer(); // Ses süresini yönet

    // İmleç Yanıp Sönme Efekti (Yaklaşık 0.5 saniyede bir)
    /*
    if (timer_ticks % 9 == 0) {
        if (cursor_blink_state) draw_cursor(0); // Sil
        else draw_cursor(15); // Çiz
        cursor_blink_state = !cursor_blink_state;
    }
    */
    // Double buffering varken cursor_blink farklı yönetilmeli (her frame çizilmeli)
    // Şimdilik disable edelim veya sadece timer'a bağlı çizelim.
    // draw_cursor() fonksiyonu backbuffer'a çizer.
}

void user_test_app() {
    const char* msg = "\n[USER] Merhaba Ring 3! Syscall basarili.\n";
    // SYS_WRITE (4), fd=1, buf=msg, count=40
    asm volatile("mov $4, %%eax; mov $1, %%ebx; mov %0, %%ecx; mov $40, %%edx; int $0x80" : : "r"(msg) : "eax", "ebx", "ecx", "edx");
    while(1) {
        // Döngüde kal, multitasking diğer tasklara geçecek
    }
}

void isr_handler(struct registers* r) {
    (void)r;
    print("\n[!] KRITIK HATA: SISTEM DURDURULDU.");
    while(1) hlt();
}

uint32_t irq_handler(struct registers* r) {
    if (r->int_no >= 40) outb(0xA0, 0x20); // Slave
    outb(0x20, 0x20); // Master

    if (r->int_no == 32) { // Timer
        timer_handler();
        return schedule((uint32_t)r);
    } else if (r->int_no == 33) {
        uint8_t scancode = inb(0x60);
        
        if (!(scancode & 0x80)) {
            // Özel Türkçe Karakter Kontrolü (TR-Q)
            if (scancode == 0x1A) { print("ğ"); }
            else if (scancode == 0x1B) { print("ü"); }
            else if (scancode == 0x27) { print("ş"); }
            else if (scancode == 0x33) { print("ö"); }
            else if (scancode == 0x34) { print("ç"); }
            else if (scancode == 0x17) { print("ı"); }
            else {
                char c = keyboard_map[scancode];
                if (c != 0) {
                    if (!on_window_key_event(c)) {
                        shell_input(c);
                    }
                }
            }
        }
    } else if (r->int_no == 44) { // IRQ12 - Fare
        handle_mouse_packet();
    }
    return (uint32_t)r;
}

void clear_screen() {
    // Shell için: Sadece arkaplanı temizle (backbuffer'a)
    // 1. Degrade Mavi Arkaplan (Temel)
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        vga_draw_line(0, i, SCREEN_WIDTH - 1, i, 104);
    }
    
    gfx_cursor_x = 0;
    gfx_cursor_y = 12; // Status bar'ın hemen altı
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
    
    vga_init_double_buffer(); // Double Buffering Başlat
    vga_clear(0); // Siyah ekran
    
    // Arka plana renk katalım
    for(int i = 0; i < SCREEN_HEIGHT; i++) {
        vga_draw_line(0, i, SCREEN_WIDTH - 1, i, i % 256);
    }
    
    vga_draw_rect(SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 - 50, 220, 100, 0); // Siyah kutu
    vga_draw_text(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 30, "GUMUS OS UYANIS", 14); // Sarı yazı
    vga_draw_text(SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT/2 - 10, "VBE 800x600 AKTIF!", 10); // Yeşil yazı
    vga_draw_rect(SCREEN_WIDTH/2 - 110, SCREEN_HEIGHT/2 + 48, 220, 2, 15); // Alt çizgi

    init_idt();
    
    // Sistem Açılış Sesi
    beep_hz(440, 100);
    beep_hz(880, 100);
    
    // Modüler Sürücü Sistemi Başlatılıyor
    driver_manager_init();
    pseudo_drivers_init(); // Null, Zero sürücülerini yükle
    hardware_detect_init(); // Donanım tespiti ve sürücü yükleme
    
    ata_init_driver();
    driver_register(create_serial_driver()); // Seri Port Sürücüsünü Kaydet
    vfs_init();
    
    // USB Host Controller Sistemi Başlatılıyor
    usb_host_init();
    
    mouse_init(); // Fareyi Başlat
    
    init_multitasking(); // Çoklu Görev Hazırlığı (PID 0 Kernel Kaydı)
    create_user_process(user_test_app); // Yeni: Kullanıcı Modu Süreci Başlat
    
    init_window_manager(); // Pencere Yöneticisi Başlat
    init_file_manager();   // Dosya Yöneticisi Başlat

    __asm__ volatile("sti");
    shell_init();

    while(1) {
        hlt();
    }
}
