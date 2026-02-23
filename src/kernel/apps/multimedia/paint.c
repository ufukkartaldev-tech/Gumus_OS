#include "paint.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "memory.h"
#include "mouse.h"
#include "fs.h"
#include "string.h"
#include "printf.h"
#include "math.h"

// Tuval Boyutları
#define CANVAS_W 120
#define CANVAS_H 80

// Paint Durumu Yapısı (En başta olmalı)
typedef struct {
    uint8_t current_color;
    uint8_t brush_size;
    uint8_t* canvas; 
    int is_drawing;
    int last_x, last_y;
} paint_t;

// Extern mouse değişkenleri (Mouse.c'den geliyor)
extern uint8_t mouse_left; 
extern int mouse_x;
extern int mouse_y;

// Dosya Kaydetme Fonksiyonu
void save_paint_image(paint_t* paint) {
    int header_size = 8;
    int data_size = CANVAS_W * CANVAS_H;
    int total_size = header_size + data_size;
    
    uint8_t* buffer = kmalloc(total_size);
    if (!buffer) return;
    
    // GUM Header: [G, U, M] [Ver] [W_Low, W_High] [H_Low, H_High]
    buffer[0] = 'G'; buffer[1] = 'U'; buffer[2] = 'M';
    buffer[3] = 1; 
    buffer[4] = CANVAS_W & 0xFF;
    buffer[5] = (CANVAS_W >> 8) & 0xFF;
    buffer[6] = CANVAS_H & 0xFF;
    buffer[7] = (CANVAS_H >> 8) & 0xFF;
    
    // Pixel verilerini kopyala
    memcpy(buffer + 8, paint->canvas, data_size);
    
    fs_write_bin("RESIM.GUM", buffer, total_size);
    kfree(buffer);
    printf("Resim RESIM.GUM olarak kaydedildi.\n");
}

// Bresenham Çizgi Algoritması
void paint_line(paint_t* paint, int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        if (x0 >= 0 && x0 < CANVAS_W && y0 >= 0 && y0 < CANVAS_H) {
            paint->canvas[y0 * CANVAS_W + x0] = paint->current_color;
            // Fırça kalınlığı (2x2 kare)
            if (paint->brush_size > 1) {
                if (x0 + 1 < CANVAS_W) paint->canvas[y0 * CANVAS_W + x0 + 1] = paint->current_color;
                if (y0 + 1 < CANVAS_H) paint->canvas[(y0 + 1) * CANVAS_W + x0] = paint->current_color;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Çizim Fonksiyonu (Refresh)
void paint_draw(window_t* win) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Yan Panel (Araçlar)
    vga_draw_rect(px + 2, py + 14, 20, win->h * 8 - 16, 7); 
    
    // Renk Paleti (8 Renk)
    uint8_t palette[] = {0, 15, 4, 2, 1, 14, 40, 5}; 
    for(int i=0; i<8; i++) {
        int cx = px + 4 + (i % 2) * 8;
        int cy = py + 16 + (i / 2) * 8;
        vga_draw_rect(cx, cy, 6, 6, palette[i]);
        if (paint->current_color == palette[i]) {
            vga_draw_rect(cx - 1, cy - 1, 8, 8, 15); // Seçili rengi beyaz çerçeve yap
            vga_draw_rect(cx, cy, 6, 6, palette[i]);
        }
    }
    
    // Kaydet Butonu
    vga_draw_rect(px + 4, py + 60, 14, 14, 8);
    vga_draw_text(px + 8, py + 64, "S", 15);
    
    // Tuvali Ekrana Bas
    int canvas_offset_x = px + 25;
    int canvas_offset_y = py + 14;
    for(int y = 0; y < CANVAS_H; y++) {
        for(int x = 0; x < CANVAS_W; x++) {
            vga_putpixel(canvas_offset_x + x, canvas_offset_y + y, paint->canvas[y * CANVAS_W + x]);
        }
    }
    vga_draw_rect(canvas_offset_x - 1, canvas_offset_y - 1, CANVAS_W + 2, CANVAS_H + 2, 0);
}

// Tıklama Yönetimi
void paint_click(window_t* win, int x, int y) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    int local_x = x - (win->x * 8);
    int local_y = y - (win->y * 8);

    // Renk Seçimi
    if (local_x >= 4 && local_x <= 20 && local_y >= 16 && local_y < 48) {
        int idx = ((local_y - 16) / 8) * 2 + ((local_x - 4) / 8);
        uint8_t palette[] = {0, 15, 4, 2, 1, 14, 40, 5};
        if (idx >= 0 && idx < 8) paint->current_color = palette[idx];
    }
    
    // Kaydet Tıklandı mı?
    if (local_x >= 4 && local_x <= 18 && local_y >= 60 && local_y <= 74) {
        save_paint_image(paint);
    }
}

// Sürekli Çizim (Update Loop)
void paint_update(window_t* win) {
    paint_t* paint = (paint_t*)win->data;
    if (!paint) return;
    
    if (mouse_left) {
        int lx = mouse_x - (win->x * 8) - 25;
        int ly = mouse_y - (win->y * 8) - 14;
        
        if (lx >= 0 && lx < CANVAS_W && ly >= 0 && ly < CANVAS_H) {
            if (paint->is_drawing) {
                paint_line(paint, paint->last_x, paint->last_y, lx, ly);
            } else {
                paint->canvas[ly * CANVAS_W + lx] = paint->current_color;
            }
            paint->last_x = lx;
            paint->last_y = ly;
            paint->is_drawing = 1;
        } else {
            paint->is_drawing = 0;
        }
    } else {
        paint->is_drawing = 0;
    }
}

void paint_close(window_t* win) {
    if (win->data) {
        paint_t* paint = (paint_t*)win->data;
        if (paint->canvas) kfree(paint->canvas);
        kfree(paint);
    }
}

void init_paint() {
    paint_t* paint = (paint_t*)kmalloc(sizeof(paint_t));
    paint->current_color = 0;
    paint->brush_size = 2;
    paint->is_drawing = 0;
    paint->canvas = (uint8_t*)kmalloc(CANVAS_W * CANVAS_H);
    memset(paint->canvas, 15, CANVAS_W * CANVAS_H); // Beyaz kağıt
    
    int win_id = create_window("Gumus-Paint", 5, 5, 20, 14, (7 << 4) | 0);
    set_window_callbacks(win_id, paint_draw, paint_click);
    set_window_data(win_id, paint, paint_close);
    set_window_update_callback(win_id, paint_update);
}