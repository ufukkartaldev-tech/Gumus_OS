#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <stdint.h>

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define LFB_INFO_ADDR 0x500

void vga_init_double_buffer();
void vga_present(); // Arka tamponu ekrana bas

void vga_putpixel(int x, int y, uint8_t color);
void vga_clear(uint8_t color);
void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void vga_draw_char(int x, int y, char c, uint8_t color);
void vga_draw_text(int x, int y, const char* str, uint8_t color);
void vga_draw_icon(int x, int y, const uint8_t* icon, int w, int h); // Ä°kon Ã‡iz
void draw_taskbar();

#endif
