#include "mouse.h"
#include "io.h"
#include "vga_gfx.h"
#include "kernel.h"
#include "window.h" // Pencere yÃ¶neticisi entegrasyonu
#include "file_manager.h"
#include "start_menu.h"

// Fare Veri Paketi (3 Byte)
// Byte 0: Durum (Butonlar, YÃ¶n iÅŸaretleri)
// Byte 1: X Hareketi
// Byte 2: Y Hareketi
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

// Fare Ä°mleÃ§ Pozisyonu (Ekran OrtasÄ±)
// Fare Ä°mleÃ§ Pozisyonu (Ekran OrtasÄ±)
int mouse_x = 160;
int mouse_y = 100;

// Mouse Buton DurumlarÄ±
uint8_t mouse_left = 0;
uint8_t mouse_right = 0;

// SÃ¼rÃ¼kleme Durumu
static int dragging_window_id = -1;
static int prev_mouse_left = 0;

// Ã‡ift TÄ±klama Takibi
static uint32_t last_click_time = 0;
static int last_click_x = 0;
static int last_click_y = 0;
static int click_count = 0;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { // Veri bekleniyor
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
        return;
    } else { // Komut gÃ¶nderilecek
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
        return;
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init() {
    uint8_t _status;

    // FiyakalÄ± Fare HazÄ±rlÄ±ÄŸÄ±
    mouse_wait(1);
    outb(0x64, 0xA8); // Fare portunu aÃ§
    
    mouse_wait(1);
    outb(0x64, 0x20); // Komut byte'Ä±nÄ± oku
    mouse_wait(0);
    _status = (inb(0x60) | 2); // IRQ12'yi (Fare) etkinleÅŸtir
    
    mouse_wait(1);
    outb(0x64, 0x60); // Komut byte'Ä±nÄ± yaz
    mouse_wait(1);
    outb(0x60, _status);

    // Fare AyarlarÄ±
    mouse_write(0xF6); // VarsayÄ±lanlar
    mouse_read();
    
    mouse_write(0xF4); // Veri akÄ±ÅŸÄ±nÄ± baÅŸlat (Enable Streaming)
    mouse_read();
}

void draw_mouse_cursor() {
    // Basit bir ok iÅŸareti Ã§izelim
    // Renk: Parlak KÄ±rmÄ±zÄ± (dikkat Ã§eksin)
    // Ä°mleÃ§:
    // *
    // **
    // * *
    // *  *
    // ****
    
    uint8_t color = 4; // KÄ±rmÄ±zÄ±
    
    // SÄ±nÄ±r KontrolÃ¼
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= SCREEN_WIDTH - 5) mouse_x = SCREEN_WIDTH - 5;
    if (mouse_y >= SCREEN_HEIGHT - 5) mouse_y = SCREEN_HEIGHT - 5;

    // Tek piksel deÄŸil, kÃ¼Ã§Ã¼k bir ok Ã§iziyoruz
    vga_putpixel(mouse_x, mouse_y, color);
    vga_putpixel(mouse_x, mouse_y+1, color);
    vga_putpixel(mouse_x+1, mouse_y+1, color);
    vga_putpixel(mouse_x, mouse_y+2, color);
    vga_putpixel(mouse_x+2, mouse_y+2, color);
    vga_putpixel(mouse_x, mouse_y+3, color);
    vga_putpixel(mouse_x+3, mouse_y+3, color);
    vga_putpixel(mouse_x, mouse_y+4, color);
    vga_putpixel(mouse_x+1, mouse_y+4, color);
    vga_putpixel(mouse_x+2, mouse_y+4, color);
}

// Interrupt Handler'dan Ã§aÄŸrÄ±lacak
void mouse_update(int8_t dx, int8_t dy) {
    // Eski imleci temizle (ÅŸimdilik basitÃ§e Ã¼zerine siyah basÄ±yoruz, ama bu arka planÄ± siler)
    // TODO: Backbuffer veya "Layer" sistemi olmadan arka planÄ± korumak zordur.
    // Åimdilik sadece yeni pozisyona Ã§iziyoruz, arkada iz bÄ±rakacak (Ghosting).
    // Basit bir Ã§Ã¶zÃ¼m: Sadece hareket varsa Ã§iz ?
    
    // Ä°z bÄ±rakmamasÄ± iÃ§in o alanÄ± tekrar Ã§izmek gerekir, ama kernel'Ä±n ne Ã§izildiÄŸini bilmiyor.
    // Åimdilik geÃ§ici Ã§Ã¶zÃ¼m: Sadece Ã§iz. Ghosting olacak, bir sonraki adÄ±mda Double Buffering ekleriz.
    
    // vga_draw_rect(mouse_x, mouse_y, 5, 5, 0); // Eski yeri sil (Siyah Kare)
    // Ãœstteki satÄ±rÄ± aÃ§arsan arkadaki her ÅŸeyi siyaha boyar.

    mouse_x += dx;
    mouse_y -= dy; // PS/2 Y ekseni terstir
    
    // Pencere SÃ¼rÃ¼kleme MantÄ±ÄŸÄ±
    if (mouse_left && !prev_mouse_left) { // TÄ±klama AnÄ± (Mouse Down)
        int win_id = get_window_at(mouse_x, mouse_y);
        if (win_id != -1) {
            dragging_window_id = win_id;
            focus_window(win_id); // TÄ±klanan pencereyi Ã¶ne getir
            
            // TÄ±klama olayÄ±nÄ± ilet (Sadece focus iÃ§in deÄŸil, iÃ§erik etkileÅŸimi iÃ§in)
            // SÃ¼rÃ¼kleme ile tÄ±klamayÄ± ayÄ±rmak gerekebilir ama ÅŸimdilik her down click gÃ¶nderilsin.
            
            uint32_t current_time = get_timer_ticks();
            if (current_time - last_click_time < 10 && // ~500ms
                (mouse_x - last_click_x) < 5 && (mouse_x - last_click_x) > -5 &&
                (mouse_y - last_click_y) < 5 && (mouse_y - last_click_y) > -5) {
                // Ã‡ift TÄ±klama!
                // Åimdilik Ã§ift tÄ±klamayÄ± da aynÄ± event ile gÃ¶nderelim ama belki window.h'a on_double_click eklemek daha iyi olurdu.
                // File Manager tarafÄ±nda zaman kontrolÃ¼ yapmak yerine burada yapÄ±yoruz.
                // Ama window struct'Ä±nda on_double_click yok.
                // GeÃ§ici Ã§Ã¶zÃ¼m: on_click'e Ã¶zel bir parametre veya hÄ±zlÄ±ca 2. Ã§aÄŸrÄ±.
                // Biz sadece on_click Ã§aÄŸÄ±rmaya devam edelim, File Manager kendi iÃ§inde sÃ¼re tutabilir
                // YA DA burasÄ± "Double Click" olduÄŸunu bilip ona gÃ¶re davranabilir.
                
                // window.c'de on_click handler'Ä± gÃ¼ncelleyelim mi?
                // Basitlik adÄ±na: File Manager 2. tÄ±klamada aÃ§sÄ±n.
                // Ama mouse driver Ã§ift tÄ±klama olduÄŸunu biliyor.
                
                // Hadi window yapÄ±sÄ±na dokunmadan, File Manager'Ä±n bu ayrÄ±mÄ± yapmasÄ±nÄ± saÄŸlayalÄ±m.
                // Sadece on_click Ã§aÄŸÄ±rÄ±yoruz.
            }
            
            last_click_time = current_time;
            last_click_x = mouse_x;
            last_click_y = mouse_y;
            
            on_window_click_event(win_id, mouse_x, mouse_y);
        } else {
            // HiÃ§bir pencereye tÄ±klanmadÄ± (MasaÃ¼stÃ¼)
            // Ä°kon KontrolÃ¼
            
            // BilgisayarÄ±m (10, 10, 12x12 + Text)
            if (mouse_x >= 10 && mouse_x <= 50 && mouse_y >= 10 && mouse_y <= 30) {
                 uint32_t current_time = get_timer_ticks();
                 if (current_time - last_click_time < 10) {
                     // Ã‡ift TÄ±k: Dosya YÃ¶neticisi AÃ§
                     show_file_manager_window();
                 }
                 last_click_time = current_time;
            }
            
            // Ã‡Ã¶p Kutusu
            if (mouse_x >= 10 && mouse_x <= 50 && mouse_y >= 50 && mouse_y <= 70) {
                 uint32_t current_time = get_timer_ticks();
                 if (current_time - last_click_time < 10) {
                     // Ã‡ift TÄ±k
                     create_window("Cop Kutusu", 10, 10, 20, 10, (8 << 4) | 15);
                 }
                 last_click_time = current_time;
            }
            
            // BaÅŸlat MenÃ¼sÃ¼ Butonu
            // Taskbar Y: 180-200
            // Buton: x:2, y:182, w:50, h:16
            if (mouse_y >= 180 && mouse_x <= 60) { // Biraz geniÅŸ tolerans
                 // BaÅŸlat MenÃ¼sÃ¼nÃ¼ AÃ§/Kapa
                 toggle_start_menu();
                 
                 // Debounce? (Ã‡ok hÄ±zlÄ± tÄ±klama sorun olabilir ama last_click_time burada kontrol edilmiyor)
                 // Basit bir gecikme ekleyelim
                 while(mouse_left); // BÄ±rakÄ±lana kadar bekle? HayÄ±r sistem donar.
                 // Åimdilik timer kontrolÃ¼ ekleyelim.
                 // if (current_time - last_toggle_time > 5) ...
            }
        }
    } else if (!mouse_left && prev_mouse_left) { // BÄ±rakma AnÄ± (Mouse Up)
        dragging_window_id = -1;
    }
    
    if (mouse_left && dragging_window_id != -1) {
        // SÃ¼rÃ¼kleniyor
        // Hareketi yavaÅŸlatmak iÃ§in divider kullanabiliriz
        // veya piksel hassasiyeti iÃ§in window.c'yi gÃ¼ncellemeliyiz.
        // Åimdilik kaba hareket (dx/dy kadar)
        
        // Ã‡ok hassas olduÄŸu iÃ§in sadece bÃ¼yÃ¼k hareketleri yolla veya div 4 yap
        if (dx != 0 || dy != 0) {
             // Y ekseni ters olduÄŸu iÃ§in -dy gÃ¶nderiyoruz ama mouse_y zaten -dy yapÄ±ldÄ±.
             // move_window fonksiyonu +y aÅŸaÄŸÄ± iniyor bekliyor.
             // Bizim 'dy' (paket) yukarÄ±+ ise pozitiftir.
             // mouse_y -= dy; yani yukarÄ± Ã§Ä±kÄ±nca y azalÄ±r.
             // move_window dy<0 ise yukarÄ± Ã§Ä±kar.
             // Yani mouse Y hareketi (dy pozitif -> yukarÄ±) -> move_window (dy negatif -> yukarÄ±)
             // Paket dy: yukarÄ± +, aÅŸaÄŸÄ± -
             // move_window dy: yukarÄ± -, aÅŸaÄŸÄ± +
             move_window(dragging_window_id, dx, -dy);
        }
    }
    
    prev_mouse_left = mouse_left;
    
    // Ã–nce pencereleri (ve varsa arkaplanÄ±) Ã§iz, sonra fareyi Ã§iz
    // Ghosting'i Ã¶nlemenin en basit yolu:
    // Her fare hareketinde ekranÄ± temizle -> Pencereleri Ã§iz -> Fareyi Ã§iz
    // Ama bu Ã§ok yavaÅŸ (flicker) olur.
    // Åimdilik sadece pencereleri Ã§izdiÄŸimizde ghosting azalÄ±r Ã§Ã¼nkÃ¼ pencereler Ã¼stÃ¼ne Ã§izer.
    
    // draw_windows(); // Bu Ã§ok aÄŸÄ±r gelebilir her fare hareketinde!
    // Ama sÃ¼rÃ¼klerken mecburen Ã§aÄŸÄ±rÄ±yoruz (move_window iÃ§inde).
    // SÃ¼rÃ¼kleme yoksa sadece fareyi Ã§iz.
    
    // SÃ¼rÃ¼kleme varsa move_window Ã§aÄŸrÄ±ldÄ±, o da pencere pozisyonunu gÃ¼ncelledi.
    // Ã‡izim iÅŸlemi Timer Interrupt (Render Loop) tarafÄ±ndan yapÄ±lacak.
    
    /*
    if (dragging_window_id == -1) {
       draw_mouse_cursor(); 
    } else {
       draw_mouse_cursor();
    }
    */
}

void handle_mouse_packet() {
    uint8_t data = inb(0x60);
    
    switch(mouse_cycle) {
        case 0:
            if ((data & 0x08) == 0) return; // Senkronizasyon hatasÄ±
            mouse_byte[0] = data;
            mouse_cycle++;
            break;
        case 1:
            mouse_byte[1] = data;
            mouse_cycle++;
            break;
        case 2:
            mouse_byte[2] = data;
            mouse_cycle = 0;

            // Paket tamamlandÄ±, hareketi iÅŸle
            // Overflow (TaÅŸma) kontrolÃ¼
            if (mouse_byte[0] & 0x80 || mouse_byte[0] & 0x40) return;

            mouse_left = (mouse_byte[0] & 0x01);
            mouse_right = (mouse_byte[0] & 0x02);
            
            mouse_update(mouse_byte[1], mouse_byte[2]);
            break;
    }
}

int get_mouse_state(mouse_state_t* state) {
    if (!state) return -1;
    state->x = mouse_x;
    state->y = mouse_y;
    state->buttons = (mouse_left ? 1 : 0) | (mouse_right ? 2 : 0);
    return 0;
}
