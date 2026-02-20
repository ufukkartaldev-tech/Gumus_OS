#include "mouse.h"
#include "io.h"
#include "vga_gfx.h"
#include "kernel.h"
#include "window.h" // Pencere yöneticisi entegrasyonu
#include "file_manager.h"
#include "start_menu.h"

// Fare Veri Paketi (3 Byte)
// Byte 0: Durum (Butonlar, Yön işaretleri)
// Byte 1: X Hareketi
// Byte 2: Y Hareketi
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

// Fare İmleç Pozisyonu (Ekran Ortası)
// Fare İmleç Pozisyonu (Ekran Ortası)
int mouse_x = 160;
int mouse_y = 100;

// Mouse Buton Durumları
uint8_t mouse_left = 0;
uint8_t mouse_right = 0;

// Sürükleme Durumu
static int dragging_window_id = -1;
static int prev_mouse_left = 0;

// Çift Tıklama Takibi
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
    } else { // Komut gönderilecek
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

    // Fiyakalı Fare Hazırlığı
    mouse_wait(1);
    outb(0x64, 0xA8); // Fare portunu aç
    
    mouse_wait(1);
    outb(0x64, 0x20); // Komut byte'ını oku
    mouse_wait(0);
    _status = (inb(0x60) | 2); // IRQ12'yi (Fare) etkinleştir
    
    mouse_wait(1);
    outb(0x64, 0x60); // Komut byte'ını yaz
    mouse_wait(1);
    outb(0x60, _status);

    // Fare Ayarları
    mouse_write(0xF6); // Varsayılanlar
    mouse_read();
    
    mouse_write(0xF4); // Veri akışını başlat (Enable Streaming)
    mouse_read();
}

void draw_mouse_cursor() {
    // Basit bir ok işareti çizelim
    // Renk: Parlak Kırmızı (dikkat çeksin)
    // İmleç:
    // *
    // **
    // * *
    // *  *
    // ****
    
    uint8_t color = 4; // Kırmızı
    
    // Sınır Kontrolü
    if (mouse_x < 0) mouse_x = 0;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_x >= SCREEN_WIDTH - 5) mouse_x = SCREEN_WIDTH - 5;
    if (mouse_y >= SCREEN_HEIGHT - 5) mouse_y = SCREEN_HEIGHT - 5;

    // Tek piksel değil, küçük bir ok çiziyoruz
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

// Interrupt Handler'dan çağrılacak
void mouse_update(int8_t dx, int8_t dy) {
    // Eski imleci temizle (şimdilik basitçe üzerine siyah basıyoruz, ama bu arka planı siler)
    // TODO: Backbuffer veya "Layer" sistemi olmadan arka planı korumak zordur.
    // Şimdilik sadece yeni pozisyona çiziyoruz, arkada iz bırakacak (Ghosting).
    // Basit bir çözüm: Sadece hareket varsa çiz ?
    
    // İz bırakmaması için o alanı tekrar çizmek gerekir, ama kernel'ın ne çizildiğini bilmiyor.
    // Şimdilik geçici çözüm: Sadece çiz. Ghosting olacak, bir sonraki adımda Double Buffering ekleriz.
    
    // vga_draw_rect(mouse_x, mouse_y, 5, 5, 0); // Eski yeri sil (Siyah Kare)
    // Üstteki satırı açarsan arkadaki her şeyi siyaha boyar.

    mouse_x += dx;
    mouse_y -= dy; // PS/2 Y ekseni terstir
    
    // Pencere Sürükleme Mantığı
    if (mouse_left && !prev_mouse_left) { // Tıklama Anı (Mouse Down)
        int win_id = get_window_at(mouse_x, mouse_y);
        if (win_id != -1) {
            dragging_window_id = win_id;
            focus_window(win_id); // Tıklanan pencereyi öne getir
            
            // Tıklama olayını ilet (Sadece focus için değil, içerik etkileşimi için)
            // Sürükleme ile tıklamayı ayırmak gerekebilir ama şimdilik her down click gönderilsin.
            
            uint32_t current_time = get_timer_ticks();
            if (current_time - last_click_time < 10 && // ~500ms
                (mouse_x - last_click_x) < 5 && (mouse_x - last_click_x) > -5 &&
                (mouse_y - last_click_y) < 5 && (mouse_y - last_click_y) > -5) {
                // Çift Tıklama!
                // Şimdilik çift tıklamayı da aynı event ile gönderelim ama belki window.h'a on_double_click eklemek daha iyi olurdu.
                // File Manager tarafında zaman kontrolü yapmak yerine burada yapıyoruz.
                // Ama window struct'ında on_double_click yok.
                // Geçici çözüm: on_click'e özel bir parametre veya hızlıca 2. çağrı.
                // Biz sadece on_click çağırmaya devam edelim, File Manager kendi içinde süre tutabilir
                // YA DA burası "Double Click" olduğunu bilip ona göre davranabilir.
                
                // window.c'de on_click handler'ı güncelleyelim mi?
                // Basitlik adına: File Manager 2. tıklamada açsın.
                // Ama mouse driver çift tıklama olduğunu biliyor.
                
                // Hadi window yapısına dokunmadan, File Manager'ın bu ayrımı yapmasını sağlayalım.
                // Sadece on_click çağırıyoruz.
            }
            
            last_click_time = current_time;
            last_click_x = mouse_x;
            last_click_y = mouse_y;
            
            on_window_click_event(win_id, mouse_x, mouse_y);
        } else {
            // Hiçbir pencereye tıklanmadı (Masaüstü)
            // İkon Kontrolü
            
            // Bilgisayarım (10, 10, 12x12 + Text)
            if (mouse_x >= 10 && mouse_x <= 50 && mouse_y >= 10 && mouse_y <= 30) {
                 uint32_t current_time = get_timer_ticks();
                 if (current_time - last_click_time < 10) {
                     // Çift Tık: Dosya Yöneticisi Aç
                     show_file_manager_window();
                 }
                 last_click_time = current_time;
            }
            
            // Çöp Kutusu
            if (mouse_x >= 10 && mouse_x <= 50 && mouse_y >= 50 && mouse_y <= 70) {
                 uint32_t current_time = get_timer_ticks();
                 if (current_time - last_click_time < 10) {
                     // Çift Tık
                     create_window("Cop Kutusu", 10, 10, 20, 10, (8 << 4) | 15);
                 }
                 last_click_time = current_time;
            }
            
            // Başlat Menüsü Butonu
            // Taskbar Y: 180-200
            // Buton: x:2, y:182, w:50, h:16
            if (mouse_y >= 180 && mouse_x <= 60) { // Biraz geniş tolerans
                 // Başlat Menüsünü Aç/Kapa
                 toggle_start_menu();
                 
                 // Debounce? (Çok hızlı tıklama sorun olabilir ama last_click_time burada kontrol edilmiyor)
                 // Basit bir gecikme ekleyelim
                 while(mouse_left); // Bırakılana kadar bekle? Hayır sistem donar.
                 // Şimdilik timer kontrolü ekleyelim.
                 // if (current_time - last_toggle_time > 5) ...
            }
        }
    } else if (!mouse_left && prev_mouse_left) { // Bırakma Anı (Mouse Up)
        dragging_window_id = -1;
    }
    
    if (mouse_left && dragging_window_id != -1) {
        // Sürükleniyor
        // Hareketi yavaşlatmak için divider kullanabiliriz
        // veya piksel hassasiyeti için window.c'yi güncellemeliyiz.
        // Şimdilik kaba hareket (dx/dy kadar)
        
        // Çok hassas olduğu için sadece büyük hareketleri yolla veya div 4 yap
        if (dx != 0 || dy != 0) {
             // Y ekseni ters olduğu için -dy gönderiyoruz ama mouse_y zaten -dy yapıldı.
             // move_window fonksiyonu +y aşağı iniyor bekliyor.
             // Bizim 'dy' (paket) yukarı+ ise pozitiftir.
             // mouse_y -= dy; yani yukarı çıkınca y azalır.
             // move_window dy<0 ise yukarı çıkar.
             // Yani mouse Y hareketi (dy pozitif -> yukarı) -> move_window (dy negatif -> yukarı)
             // Paket dy: yukarı +, aşağı -
             // move_window dy: yukarı -, aşağı +
             move_window(dragging_window_id, dx, -dy);
        }
    }
    
    prev_mouse_left = mouse_left;
    
    // Önce pencereleri (ve varsa arkaplanı) çiz, sonra fareyi çiz
    // Ghosting'i önlemenin en basit yolu:
    // Her fare hareketinde ekranı temizle -> Pencereleri çiz -> Fareyi çiz
    // Ama bu çok yavaş (flicker) olur.
    // Şimdilik sadece pencereleri çizdiğimizde ghosting azalır çünkü pencereler üstüne çizer.
    
    // draw_windows(); // Bu çok ağır gelebilir her fare hareketinde!
    // Ama sürüklerken mecburen çağırıyoruz (move_window içinde).
    // Sürükleme yoksa sadece fareyi çiz.
    
    // Sürükleme varsa move_window çağrıldı, o da pencere pozisyonunu güncelledi.
    // Çizim işlemi Timer Interrupt (Render Loop) tarafından yapılacak.
    
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
            if ((data & 0x08) == 0) return; // Senkronizasyon hatası
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

            // Paket tamamlandı, hareketi işle
            // Overflow (Taşma) kontrolü
            if (mouse_byte[0] & 0x80 || mouse_byte[0] & 0x40) return;

            mouse_left = (mouse_byte[0] & 0x01);
            mouse_right = (mouse_byte[0] & 0x02);
            
            mouse_update(mouse_byte[1], mouse_byte[2]);
            break;
    }
}
