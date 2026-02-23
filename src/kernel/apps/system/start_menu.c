#include "start_menu.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "file_manager.h"
#include "snake.h"
#include "string.h"
#include "mouse.h"
#include "calculator.h"
#include "paint.h"

void start_menu_update(window_t* win);

// MenÃ¼ Ã–ÄŸeleri
static const char* menu_items[] = {
    "Dosyalar",
    "Yilan Oyunu",
    "Hesap Makinesi",
    "Paint",
    "Hakkinda",
    "Kapat"
};
static int menu_item_count = 6;
static int start_menu_id = -1;
static int last_hover_index = -1;

// Mouse pozisyonuna eriÅŸim iÃ§in (Hover efekti)
extern int mouse_x;
extern int mouse_y;

void start_menu_draw(window_t* win) {
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Gri Arka Plan
    vga_draw_rect(px + 1, py + 12, (win->w * 8) - 2, (win->h * 8) - 13, 7);
    
    // Sol Yan Åerit (Mavi)
    vga_draw_rect(px + 1, py + 12, 20, (win->h * 8) - 13, 1);
    
    // "GUMUS" dkey yazÄ±sÄ±
    // vga_draw_char(px + 7, py + 20, 'G', 15);
    // vga_draw_char(px + 7, py + 30, 'U', 15);
    // ... (Basit font ile zor, ÅŸimdilik boÅŸver)
    
    // "GUMUS" dkey yazÄ±sÄ±
    // vga_draw_char(px + 7, py + 20, 'G', 15);
    // ...
    
    for (int i = 0; i < menu_item_count; i++) {
        int item_y = py + 15 + (i * 20);
        
        // Hover Efekti
        // Mouse bu Ã¶ÄŸenin Ã¼zerinde mi?
        // Ã–ÄŸenin alanÄ±:
        // x: px + 25 (ikon) -> px + 150 (geniÅŸlik sonu)
        // y: item_y -> item_y + 16
        
        // Mouse mutlak koordinatlarÄ±
        uint8_t text_color = 0; // Siyah
        uint8_t bg_color = 7; // Gri
        
        if (mouse_x >= px + 25 && mouse_x <= px + 150 &&
            mouse_y >= item_y && mouse_y <= item_y + 16) {
            // Hover!
            bg_color = 1; // Mavi
            text_color = 15; // Beyaz
            
            // SeÃ§ili alan arka planÄ±
            vga_draw_rect(px + 23, item_y - 2, 130, 20, bg_color);
        }
        
        // Ä°kon Yeri (Mockup renkli kutu)
        vga_draw_rect(px + 25, item_y, 16, 16, 15); // Beyaz Kutu
        
        // YazÄ±
        vga_draw_text(px + 45, item_y + 4, menu_items[i], text_color);
    }
}

void start_menu_click(window_t* win, int x, int y) {
    // x, y pencere iÃ§i (piksel)
    // BaÅŸlÄ±k Ã§ubuÄŸu 12px
    if (y < 12) return;
    
    int content_y = y - 12; // Ä°Ã§erik alanÄ±
    
    // Her Ã¶ÄŸe 20 piksel yÃ¼ksekliÄŸinde, 3 piksel padding ile baÅŸlÄ±yor (15)
    // item_y = 15 + i*20
    // click_y = content_y
    
    if (content_y < 3) return; // Ãœst boÅŸluk
    
    int index = (content_y - 3) / 20;
    
    if (index >= 0 && index < menu_item_count) {
        // TÄ±klanan Ã–ÄŸeyi Ä°ÅŸle
        if (index == 0) { // Dosyalar
            show_file_manager_window();
        } else if (index == 1) { // YÄ±lan
            init_snake_game();
        } else if (index == 2) { // Hesap Makinesi
            init_calculator();
        } else if (index == 3) { // Paint
            init_paint();
        } else if (index == 4) { // HakkÄ±nda
            create_window("Hakkinda", 10, 8, 25, 10, (1 << 4) | 15);
            // HakkÄ±nda penceresinin iÃ§eriÄŸi iÃ§in draw callback lazÄ±m ama 
            // ÅŸimdilik boÅŸ pencere aÃ§Ä±yoruz.
        } else if (index == 5) { // Kapat
             // Sistemi durdur veya reset at
             // APM/ACPI olmadan shutdown zordur.
             // qemu debug exit yapÄ±labilir ama gerÃ§ek donanÄ±mda reset.
             // outb(0x64, 0xFE); // CPU Reset
             vga_clear(0);
             vga_draw_text(100, 90, "Bilgisayari Kapatabilirsiniz.", 15);
             while(1) __asm__ volatile("hlt");
        }
        
        // MenÃ¼yÃ¼ Kapat (SeÃ§im yapÄ±ldÄ±)
        close_window(win->id);
        start_menu_id = -1;
    }
}

void show_start_menu() {
    // MenÃ¼ zaten aÃ§Ä±k mÄ± kontrol et?
    // Åimdilik her seferinde yeni aÃ§Ä±yoruz, toggle mantÄ±ÄŸÄ± mouse.c'de olabilir.
    
    // x: 0, y: 180 (taskbar) - yÃ¼kseklik
    // w: 20 (char) -> 160 px
    // h: 15 (char) -> 120 px
    // y = 200 - 20 (taskbar) - 120 = 60
    
    int win_h_chars = (menu_item_count * 3) + 2; // Kabaca yÃ¼kseklik
    int win_h_px = win_h_chars * 8;
    int y_char = (200 - 20 - win_h_px) / 8;
    if (y_char < 0) y_char = 0;
    
    // 200 piksel yÃ¼kseklik. Taskbar 20.
    // MenÃ¼ 100 piksel olsun.
    // y = 80.
    
    // y = 80.
    
    // EÄŸer zaten aÃ§Ä±ksa kapat (Toggle mantÄ±ÄŸÄ±nÄ± burada da kontrol edelim ama ayrÄ± fonksiyonda daha temiz)
    // create_window yeni ID dÃ¶ner.
    
    start_menu_id = create_window("GUMUS", 0, 10, 20, 12, (1 << 4) | 15); // Mavi BaÅŸlÄ±k
    set_window_callbacks(start_menu_id, start_menu_draw, start_menu_click);
    
    // MenÃ¼yÃ¼ sÃ¼rekli gÃ¼ncellemek iÃ§in update callback ekleyelim mi?
    // Mouse hareket edince hover efekti iÃ§in yeniden Ã§izilmesi lazÄ±m.
    // Evet!
    set_window_update_callback(start_menu_id, start_menu_update); // AÅŸaÄŸÄ±da tanÄ±mlayacaÄŸÄ±z
    last_hover_index = -1; // Reset
}

// Hover efekti iÃ§in update callback
// Double Buffering ile her frame Ã§izildiÄŸi iÃ§in,
// Logic update'e gerek kalmadÄ±, Ã§izim (start_menu_draw) her ÅŸeyi hallediyor.
void start_menu_update(window_t* win) {
    // BoÅŸ
}

int is_start_menu_open() {
    // Pencere aÃ§Ä±k mÄ± kontrol et (Daha saÄŸlam yol: window manager'a sor)
    // Åimdilik ID Ã¼zerinden
    if (start_menu_id == -1) return 0;
    // Window manager'dan kontrol etmemiz lazÄ±m aslÄ±nda, Ã§Ã¼nkÃ¼ kullanÄ±cÄ± X ile kapatmÄ±ÅŸ olabilir.
    // Basitlik adÄ±na: X ile kapatÄ±ldÄ±ÄŸÄ±nda ID'yi sÄ±fÄ±rlamalÄ±yÄ±z. (OnClose callback lazÄ±m)
    // Ama ÅŸimdilik sadece ID != -1 ise aÃ§Ä±k varsayalÄ±m.
    return 1;
}

void toggle_start_menu() {
    if (is_start_menu_open()) {
        close_window(start_menu_id);
        start_menu_id = -1;
    } else {
        show_start_menu();
    }
}
