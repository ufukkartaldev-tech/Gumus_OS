#include "start_menu.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "file_manager.h"
#include "snake.h"
#include "string.h"
#include "mouse.h"

void start_menu_update(window_t* win);

// Menü Öğeleri
static const char* menu_items[] = {
    "Dosyalar",
    "Yilan Oyunu",
    "Hakkinda",
    "Kapat"
};
#include "calculator.h"
#include "paint.h"

// Menü Öğeleri
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

// Mouse pozisyonuna erişim için (Hover efekti)
extern int mouse_x;
extern int mouse_y;

void start_menu_draw(window_t* win) {
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Gri Arka Plan
    vga_draw_rect(px + 1, py + 12, (win->w * 8) - 2, (win->h * 8) - 13, 7);
    
    // Sol Yan Şerit (Mavi)
    vga_draw_rect(px + 1, py + 12, 20, (win->h * 8) - 13, 1);
    
    // "GUMUS" dkey yazısı
    // vga_draw_char(px + 7, py + 20, 'G', 15);
    // vga_draw_char(px + 7, py + 30, 'U', 15);
    // ... (Basit font ile zor, şimdilik boşver)
    
    // "GUMUS" dkey yazısı
    // vga_draw_char(px + 7, py + 20, 'G', 15);
    // ...
    
    for (int i = 0; i < menu_item_count; i++) {
        int item_y = py + 15 + (i * 20);
        
        // Hover Efekti
        // Mouse bu öğenin üzerinde mi?
        // Öğenin alanı:
        // x: px + 25 (ikon) -> px + 150 (genişlik sonu)
        // y: item_y -> item_y + 16
        
        // Mouse mutlak koordinatları
        uint8_t text_color = 0; // Siyah
        uint8_t bg_color = 7; // Gri
        
        if (mouse_x >= px + 25 && mouse_x <= px + 150 &&
            mouse_y >= item_y && mouse_y <= item_y + 16) {
            // Hover!
            bg_color = 1; // Mavi
            text_color = 15; // Beyaz
            
            // Seçili alan arka planı
            vga_draw_rect(px + 23, item_y - 2, 130, 20, bg_color);
        }
        
        // İkon Yeri (Mockup renkli kutu)
        vga_draw_rect(px + 25, item_y, 16, 16, 15); // Beyaz Kutu
        
        // Yazı
        vga_draw_text(px + 45, item_y + 4, menu_items[i], text_color);
    }
}

void start_menu_click(window_t* win, int x, int y) {
    // x, y pencere içi (piksel)
    // Başlık çubuğu 12px
    if (y < 12) return;
    
    int content_y = y - 12; // İçerik alanı
    
    // Her öğe 20 piksel yüksekliğinde, 3 piksel padding ile başlıyor (15)
    // item_y = 15 + i*20
    // click_y = content_y
    
    if (content_y < 3) return; // Üst boşluk
    
    int index = (content_y - 3) / 20;
    
    if (index >= 0 && index < menu_item_count) {
        // Tıklanan Öğeyi İşle
        if (index == 0) { // Dosyalar
            show_file_manager_window();
        } else if (index == 1) { // Yılan
            init_snake_game();
        } else if (index == 2) { // Hesap Makinesi
            init_calculator();
        } else if (index == 3) { // Paint
            init_paint();
        } else if (index == 4) { // Hakkında
            create_window("Hakkinda", 10, 8, 25, 10, (1 << 4) | 15);
            // Hakkında penceresinin içeriği için draw callback lazım ama 
            // şimdilik boş pencere açıyoruz.
        } else if (index == 5) { // Kapat
             // Sistemi durdur veya reset at
             // APM/ACPI olmadan shutdown zordur.
             // qemu debug exit yapılabilir ama gerçek donanımda reset.
             // outb(0x64, 0xFE); // CPU Reset
             vga_clear(0);
             vga_draw_text(100, 90, "Bilgisayari Kapatabilirsiniz.", 15);
             while(1) __asm__ volatile("hlt");
        }
        
        // Menüyü Kapat (Seçim yapıldı)
        close_window(win->id);
        start_menu_id = -1;
    }
}

void show_start_menu() {
    // Menü zaten açık mı kontrol et?
    // Şimdilik her seferinde yeni açıyoruz, toggle mantığı mouse.c'de olabilir.
    
    // x: 0, y: 180 (taskbar) - yükseklik
    // w: 20 (char) -> 160 px
    // h: 15 (char) -> 120 px
    // y = 200 - 20 (taskbar) - 120 = 60
    
    int win_h_chars = (menu_item_count * 3) + 2; // Kabaca yükseklik
    int win_h_px = win_h_chars * 8;
    int y_char = (200 - 20 - win_h_px) / 8;
    if (y_char < 0) y_char = 0;
    
    // 200 piksel yükseklik. Taskbar 20.
    // Menü 100 piksel olsun.
    // y = 80.
    
    // y = 80.
    
    // Eğer zaten açıksa kapat (Toggle mantığını burada da kontrol edelim ama ayrı fonksiyonda daha temiz)
    // create_window yeni ID döner.
    
    start_menu_id = create_window("GUMUS", 0, 10, 20, 12, (1 << 4) | 15); // Mavi Başlık
    set_window_callbacks(start_menu_id, start_menu_draw, start_menu_click);
    
    // Menüyü sürekli güncellemek için update callback ekleyelim mi?
    // Mouse hareket edince hover efekti için yeniden çizilmesi lazım.
    // Evet!
    set_window_update_callback(start_menu_id, start_menu_update); // Aşağıda tanımlayacağız
    last_hover_index = -1; // Reset
}

// Hover efekti için update callback
static int last_hover_index = -1;

// Hover efekti için update callback
// Double Buffering ile her frame çizildiği için,
// Logic update'e gerek kalmadı, çizim (start_menu_draw) her şeyi hallediyor.
void start_menu_update(window_t* win) {
    // Boş
}

int is_start_menu_open() {
    // Pencere açık mı kontrol et (Daha sağlam yol: window manager'a sor)
    // Şimdilik ID üzerinden
    if (start_menu_id == -1) return 0;
    // Window manager'dan kontrol etmemiz lazım aslında, çünkü kullanıcı X ile kapatmış olabilir.
    // Basitlik adına: X ile kapatıldığında ID'yi sıfırlamalıyız. (OnClose callback lazım)
    // Ama şimdilik sadece ID != -1 ise açık varsayalım.
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
