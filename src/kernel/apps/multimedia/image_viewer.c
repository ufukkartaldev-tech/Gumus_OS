#include "image_viewer.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "memory.h"
#include "fs.h"
#include "paint.h" // Sadece CANVAS_W/H için gerekirse, ama burada manuel define edebiliriz.

#define IMG_W 120
#define IMG_H 80

typedef struct {
    uint8_t* file_buffer; // Dosyanın tamamı (Free etmek için)
    uint8_t* image_data;  // Pixel verisinin başlangıcı
    int width;
    int height;
    char filename[13];
} image_viewer_t;

void image_viewer_draw(window_t* win) {
    image_viewer_t* viewer = (image_viewer_t*)win->data;
    if (!viewer) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Görüntüleme Alanı
    int view_x = px + 5;
    int view_y = py + 15;
    
    int w = viewer->width;
    int h = viewer->height;
    
    // Arkaplan
    vga_draw_rect(view_x - 1, view_y - 1, w + 2, h + 2, 0); // Siyah Çerçeve
    // vga_draw_rect(view_x, view_y, w, h, 15); // Beyaz Arkaplan (Gerek yok, üzerine çiziyoruz)
    
    if (viewer->image_data) {
        for(int y=0; y<h; y++) {
            for(int x=0; x<w; x++) {
                vga_putpixel(view_x + x, view_y + y, viewer->image_data[y * w + x]);
            }
        }
    } else {
        vga_draw_text(view_x + 10, view_y + 35, "Resim Yuklenemedi", 4);
    }
    
    // Dosya Adı (Alt Bar)
    vga_draw_text(px + 10, py + h + 20, viewer->filename, 0);
}

void image_viewer_close(window_t* win) {
    if (win->data) {
        image_viewer_t* viewer = (image_viewer_t*)win->data;
        if (viewer->file_buffer) kfree(viewer->file_buffer);
        kfree(win->data);
    }
}

void init_image_viewer(const char* filename) {
    image_viewer_t* viewer = (image_viewer_t*)kmalloc(sizeof(image_viewer_t));
    if (!viewer) return;
    
    // Dosya Adını Kopyala
    int i;
    for(i=0; i<12 && filename[i]; i++) viewer->filename[i] = filename[i];
    viewer->filename[i] = '\0';
    
    // Resmi Oku
    uint32_t size;
    uint8_t* buffer = fs_read_bin(filename, &size);
    viewer->file_buffer = buffer;
    
    if (buffer) {
        // GUM Header Kontrolü
        if (buffer[0] == 'G' && buffer[1] == 'U' && buffer[2] == 'M') {
            // GUM Formatı
            viewer->width = buffer[4] | (buffer[5] << 8);
            viewer->height = buffer[6] | (buffer[7] << 8);
            viewer->image_data = buffer + 8; // Header'ı atla
        } else {
            // Raw Format (Eski)
            viewer->width = 120;
            viewer->height = 80;
            viewer->image_data = buffer;
        }
    } else {
        viewer->image_data = 0;
        viewer->width = 100;
        viewer->height = 50;
    }

    // Pencere Boyutu dinamik olmalı
    int win_w = (viewer->width / 8) + 4;
    if (win_w < 15) win_w = 15;
    int win_h = (viewer->height / 8) + 6;
    
    // Pencere Oluştur
    int win_id = create_window("Resim Gost.", 5, 5, win_w, win_h, (7 << 4) | 0);
    
    set_window_callbacks(win_id, image_viewer_draw, 0); // Click handler yok
    set_window_data(win_id, viewer, image_viewer_close);
}
