#include "paint.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "memory.h"
#include "mouse.h"
#include "fs.h"
#include "string.h" // itoa için

void save_paint_image(paint_t* paint) {
    // GUM Dosya Formatı
    // [Signature:3 "GUM"] [Ver:1] [W:2] [H:2] [Data...]
    
    int header_size = 8;
    int data_size = CANVAS_W * CANVAS_H;
    int total_size = header_size + data_size;
    
    uint8_t* buffer = kmalloc(total_size);
    if (!buffer) return;
    
    // Header Doldur
    buffer[0] = 'G'; buffer[1] = 'U'; buffer[2] = 'M';
    buffer[3] = 1; // Sürüm
    // Width (Little Endian)
    buffer[4] = CANVAS_W & 0xFF;
    buffer[5] = (CANVAS_W >> 8) & 0xFF;
    // Height
    buffer[6] = CANVAS_H & 0xFF;
    buffer[7] = (CANVAS_H >> 8) & 0xFF;
    
    // Pixel Data Kopyala
    // paint->canvas zaten linear
    // memcpy(buffer + 8, paint->canvas, data_size); 
    // memcpy için include string.h lazım, zaten var.
    // Manuel kopyalama daha güvenli (bağımlılık yoksa)
    uint8_t* dest = buffer + 8;
    uint8_t* src = paint->canvas;
    for(int i=0; i<data_size; i++) dest[i] = src[i];
    
    fs_write_bin("RESIM.GUM", buffer, total_size);
    
    kfree(buffer);
}

// Paint Durumu
typedef struct {
    uint8_t current_color;
    uint8_t brush_size;
    uint8_t* canvas; // 100x100 tuval
    int is_drawing;
    int last_x, last_y; // Çizgi interpolasyonu için
} paint_t;

#define CANVAS_W 120
#define CANVAS_H 80

void paint_draw(window_t* win) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Araç Çubuğu (Sol)
    vga_draw_rect(px + 2, py + 14, 20, win->h * 8 - 16, 7); // Gri
    
    // Renk Paleti
    uint8_t palette[] = {0, 15, 4, 2, 1, 14, 40, 5}; // Siyah, Beyaz, Kırmızı, Yeşil, Mavi, Sarı, Turuncu, Mor
    for(int i=0; i<8; i++) {
        int cx = px + 4 + (i%2)*8;
        int cy = py + 16 + (i/2)*8;
        vga_draw_rect(cx, cy, 6, 6, palette[i]);
        if (paint->current_color == palette[i]) {
            vga_draw_rect(cx, cy, 6, 6, palette[i]); 
            vga_draw_rect(cx-1, cy-1, 8, 8, 15); // Seçili çerçeve
            vga_draw_rect(cx, cy, 6, 6, palette[i]); // Üstüne tekrar çiz
        }
    }
    
    // Kaydet Butonu (Disk İkonu basitçe 'S')
    vga_draw_rect(px + 4, py + 60, 14, 14, 8); // Gri Buton
    vga_draw_rect(px + 4, py + 60, 14, 1, 15); // Hi
    vga_draw_text(px + 8, py + 64, "S", 15); // Save
    
    // Tuval Alanı
    int canvas_x = px + 25;
    int canvas_y = py + 14;
    
    // Tuval Arkaplanı (Beyaz)
    // vga_draw_rect(canvas_x, canvas_y, CANVAS_W, CANVAS_H, 15); 
    // Ancak paint->canvas verisinden çizmeliyiz
    
    if (paint->canvas) {
        for(int y=0; y<CANVAS_H; y++) {
            for(int x=0; x<CANVAS_W; x++) {
                // backbuffer'a çiz
                // Tuval koordinatı -> Ekran koordinatı
                vga_putpixel(canvas_x + x, canvas_y + y, paint->canvas[y * CANVAS_W + x]);
            }
        }
    }
    
    // Çerçeve
    vga_draw_rect(canvas_x - 1, canvas_y - 1, CANVAS_W + 2, CANVAS_H + 2, 0); // Siyah Çerçeve
}

// Basit Çizgi Algoritması (Bresenham basitleştirilmiş)
void paint_line(paint_t* paint, int x0, int y0, int x1, int y1) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;
    
    while (1) {
        if (x0 >= 0 && x0 < CANVAS_W && y0 >= 0 && y0 < CANVAS_H) {
             paint->canvas[y0 * CANVAS_W + x0] = paint->current_color;
             // Fırça boyutu (basit kare)
             if (paint->brush_size > 1) {
                 if (x0+1 < CANVAS_W) paint->canvas[y0 * CANVAS_W + x0+1] = paint->current_color;
                 if (y0+1 < CANVAS_H) paint->canvas[(y0+1) * CANVAS_W + x0] = paint->current_color;
                 if (x0+1 < CANVAS_W && y0+1 < CANVAS_H) paint->canvas[(y0+1) * CANVAS_W + x0+1] = paint->current_color;
             }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

void paint_click(window_t* win, int x, int y) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    // Araç Çubuğu Kontrolü
    if (x >= 2 && x <= 22 && y >= 14) {
        // Renk Seçimi
        if (y >= 16 && y < 16 + 32) {
             int i = ((y - 16) / 8) * 2 + ((x - 4) / 8);
             uint8_t palette[] = {0, 15, 4, 2, 1, 14, 40, 5};
             if (i >= 0 && i < 8) {
                 paint->current_color = palette[i];
             }
        }
        
        // Kaydet Butonu (y: 60-74)
        if (y >= 60 && y <= 74) {
            // Kaydet
            // Dosya adı: RESIM.IMG
            // Boyut: CANVAS_W * CANVAS_H
            if (paint->canvas) {
                // fs_write_bin lazım ama fs.h eklemek gerek
                // Paint.c içinde include yoksa ekle.
                // Ve fonksiyon çağrısı.
                // Hack: Declare extern here or include fs.h
                // Let's rely on include at top (will add it).
                
                // Farklı isimlerle kaydetmek için sayaç? Yok, üzerine yazsın şimdilik.
                // Veya RESIM1, RESIM2...
                // Basitlik: "RESIM.RAW"
                
                // fs_write_bin("RESIM.RAW", paint->canvas, CANVAS_W * CANVAS_H);
                // Cannot call directly if not included.
                // Will fix includes.
                save_paint_image(paint);
            }
        }
    }
    
    // Tuval Kontrolü
    int canvas_x = 25;
    int canvas_y = 14;
    
    if (x >= canvas_x && x < canvas_x + CANVAS_W &&
        y >= canvas_y && y < canvas_y + CANVAS_H) {
        
        int draw_x = x - canvas_x;
        int draw_y = y - canvas_y;
        
        // Sadece nokta koyma, mouse_update sürekli çağırmıyor.
        // Ama window timer update çağırabilir mi? Hayır.
        // Mouse eventleri sadece değişince geliyor.
        // paint_click sadece "click" anında gelir. Sürükleme için?
        // window manager sürükleme (drag) bilgisini "click" olarak göndermiyor.
        // on_click sadece "down" anında tetikleniyor.
        
        // Çözüm 1: Window Manager'a on_drag eklemek.
        // Çözüm 2: Mouse sol basılı olduğu sürece, mouse handler'ın sürekli event göndermesi.
        // Mouse handler'ı değiştirmiştik, sürükleme sadece pencere taşıma içindi.
        
        // "Gümüş-Formatında" çizmek istiyorsak, mouse driver pencere içeriğine sürükleme (drag) event'i göndermeli.
        // Mouse.c -> mouse_update -> if(mouse_left) -> on_window_drag() ?
        // Yoksa on_window_click sürekli mi çağrılmalı?
        
        // Basitlik (Hack): paint_update fonksiyonu (timer) mouse sol basılıysa ve koordinat içindeyse çizer!
        // Mouse left global değişkendi (mouse.h).
        
        // Evet! paint_update kullanalım.
        // Ama "Click" anında da bir nokta koyalım.
        
        if (draw_x >= 0 && draw_x < CANVAS_W && draw_y >= 0 && draw_y < CANVAS_H) {
             paint->canvas[draw_y * CANVAS_W + draw_x] = paint->current_color;
             paint->last_x = draw_x;
             paint->last_y = draw_y;
             paint->is_drawing = 1;
        }
    }
}

extern uint8_t mouse_left; // Mouse global değişkeni

void paint_update(window_t* win) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    if (mouse_left) {
        // Mouse global koordinatlarını pencereye çevir
        int px = win->x * 8;
        int py = win->y * 8;
        int local_x = mouse_x - px;
        int local_y = mouse_y - py;
        
        // Tuval alanı
        int canvas_x = 25;
        int canvas_y = 14;
        
        int draw_x = local_x - canvas_x;
        int draw_y = local_y - canvas_y;
        
        if (draw_x >= 0 && draw_x < CANVAS_W && draw_y >= 0 && draw_y < CANVAS_H) {
            // Eğer önceki frame'de çiziyorsak çizgi çek
            if (paint->is_drawing) {
                paint_line(paint, paint->last_x, paint->last_y, draw_x, draw_y);
            } else {
                paint->canvas[draw_y * CANVAS_W + draw_x] = paint->current_color;
            }
            paint->last_x = draw_x;
            paint->last_y = draw_y;
            paint->is_drawing = 1;
        } else {
            paint->is_drawing = 0;
        }
    } else {
        paint->is_drawing = 0;
    }
    
    // Double buffer render loop her şeyi çizdiği için explicit draw gerekmez.
}

void paint_close(window_t* win) {
    if (win->data) {
        paint_t* paint = (paint_t*)win->data;
        if (paint->canvas) kfree(paint->canvas);
        kfree(win->data);
    }
}

void init_paint() {
    paint_t* paint = (paint_t*)kmalloc(sizeof(paint_t));
    paint->current_color = 0; // Siyah
    paint->brush_size = 2;
    paint->is_drawing = 0;
    paint->canvas = (uint8_t*)kmalloc(CANVAS_W * CANVAS_H);
    
    // Tuvali Beyaz Yap
    for(int i=0; i<CANVAS_W * CANVAS_H; i++) paint->canvas[i] = 15;
    
    // 160x100 pencere (20x13 char)
    int win_id = create_window("Gumus-Paint", 5, 5, 20, 14, (7 << 4) | 0); // Gri
    
    set_window_callbacks(win_id, paint_draw, paint_click);
    set_window_data(win_id, paint, paint_close);
    set_window_update_callback(win_id, paint_update); // Sürekli çizim için
}
