#include "vga_gfx.h"
#include "memory.h"
#include "string.h" // memcpy için

static uint8_t* backbuffer = 0;

static uint8_t* get_vram() {
    uint32_t* lfb_ptr = (uint32_t*)LFB_INFO_ADDR;
    return (uint8_t*)(*lfb_ptr);
}

void vga_init_double_buffer() {
    backbuffer = (uint8_t*)kmalloc(SCREEN_WIDTH * SCREEN_HEIGHT);
    if (!backbuffer) return; 
    memset(backbuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
}

void vga_present() {
    if (!backbuffer) return;
    memcpy(get_vram(), backbuffer, SCREEN_WIDTH * SCREEN_HEIGHT);
}

// Minimal 8x8 Font (Rakamlar ve temel karakterler eklendi)
static uint8_t font8x8_basic[128][8] = {
    ['A'] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00},
    ['B'] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    ['U'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['M'] = {0x66, 0x7E, 0x7E, 0x66, 0x66, 0x66, 0x66, 0x00},
    ['S'] = {0x3C, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x3C, 0x00},
    ['O'] = {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00},
    ['Y'] = {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['N'] = {0x66, 0x6E, 0x7E, 0x76, 0x66, 0x66, 0x66, 0x00},
    ['K'] = {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00},
    ['T'] = {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
    ['F'] = {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['P'] = {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00},
    ['V'] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['0'] = {0x3C, 0x66, 0x6E, 0x7E, 0x76, 0x66, 0x3C, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    ['2'] = {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00},
    ['3'] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
    ['4'] = {0x1C, 0x3C, 0x6C, 0x7E, 0x0C, 0x0C, 0x0C, 0x00},
    ['5'] = {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00},
    ['6'] = {0x1C, 0x30, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00},
    ['7'] = {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},
    ['8'] = {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00},
    ['9'] = {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00},
    [':'] = {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00},
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18},
};

void vga_putpixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    
    if (backbuffer) {
        backbuffer[y * SCREEN_WIDTH + x] = color;
    } else {
        get_vram()[y * SCREEN_WIDTH + x] = color;
    }
}

void vga_clear(uint8_t color) {
    if (backbuffer) {
         memset(backbuffer, color, SCREEN_WIDTH * SCREEN_HEIGHT);
    } else {
        memset(get_vram(), color, SCREEN_WIDTH * SCREEN_HEIGHT);
    }
}

void vga_draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            vga_putpixel(x + i, y + j, color);
        }
    }
}

void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = x2 - x1; if (dx < 0) dx = -dx;
    int dy = y2 - y1; if (dy < 0) dy = -dy;
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        vga_putpixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void vga_draw_char(int x, int y, char c, uint8_t color) {
    if ((uint8_t)c > 127) return;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (font8x8_basic[(uint8_t)c][i] & (1 << (7 - j))) {
                vga_putpixel(x + j, y + i, color);
            }
        }
    }
}

// Görev Çubuğu Çizimi
void draw_taskbar() {
    int y = SCREEN_HEIGHT - 20;
    
    // Gri Arka Plan (Win95 Style)
    vga_draw_rect(0, y, SCREEN_WIDTH, 20, 7); // Light Grey
    
    // Üst Çizgi (Beyaz - 3D Efekt)
    vga_draw_line(0, y, SCREEN_WIDTH - 1, y, 15);
    
    // "Başlat" Butonu (Basit Gümüş Buton)
    vga_draw_rect(2, y + 2, 50, 16, 8); // Gölge
    vga_draw_rect(2, y + 2, 48, 14, 7); // Buton Yüzü
    
    // 3D Buton Kenarları
    vga_draw_line(2, y + 2, 50, y + 2, 15); // Üst Beyaz
    vga_draw_line(2, y + 2, 2, y + 16, 15); // Sol Beyaz
    vga_draw_line(50, y + 2, 50, y + 16, 0); // Sağ Siyah
    vga_draw_line(2, y + 16, 50, y + 16, 0); // Alt Siyah
    
    vga_draw_text(8, y + 6, "GUMUS", 0); // Siyah Yazı
    
    // Saat Alanı (Sağ Alt)
    vga_draw_rect(SCREEN_WIDTH - 60, y + 4, 55, 14, 15); // Beyaz Kutu
    vga_draw_line(SCREEN_WIDTH - 60, y + 4, SCREEN_WIDTH - 6, y + 4, 8); // Üst Gölge
    vga_draw_line(SCREEN_WIDTH - 60, y + 4, SCREEN_WIDTH - 60, y + 18, 8); // Sol Gölge
}

void vga_draw_text(int x, int y, const char* str, uint8_t color) {
    while (*str) {
        vga_draw_char(x, y, *str++, color);
        x += 8;
    }
}
    }
}

// Basit ikon çizimi (0 = Şeffaf)
void vga_draw_icon(int x, int y, const uint8_t* icon, int w, int h) {
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            uint8_t color = icon[i * w + j];
            if (color != 0) { // Şeffaflık (0 ise çizme) (Varsayılan şeffaf renk 0 olsun)
                // Ama siyah (0) çizebilmek için şeffaf rengi 255 yapmak daha iyi olabilir.
                // Şimdilik 0 renk kullanılmıyor (Siyah).
                vga_putpixel(x + j, y + i, color);
            }
        }
    }
}
