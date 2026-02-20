#include "vesa_vbe.h"
#include "../core/memory.h"
#include "../core/io.h"
#include "../core/string.h"
#include "../core/printf.h"

static vesa_driver_t vesa_driver;
static int vesa_initialized = 0;

// BIOS interrupt 0x10 wrapper
static int vbe_bios_call(uint16_t ax, uint16_t bx, uint16_t cx, uint16_t dx, uint16_t es, uint16_t di) {
    // Bu fonksiyon gerçek BIOS interrupt çağrısı implement etmeli
    // Şimdilik simülasyon
    return 0x004F; // Success
}

// VESA Driver Functions
static int vesa_driver_init(void) {
    printf("VESA/VBE sürücüsü başlatılıyor...\n");
    return vesa_init();
}

static int vesa_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    // Framebuffer'den oku
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    if (offset + size > vesa_driver.framebuffer_size) {
        size = vesa_driver.framebuffer_size - offset;
    }
    
    memcpy(buffer, vesa_driver.framebuffer + offset, size);
    return size;
}

static int vesa_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    // Framebuffer'e yaz
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    if (offset + size > vesa_driver.framebuffer_size) {
        size = vesa_driver.framebuffer_size - offset;
    }
    
    memcpy(vesa_driver.framebuffer + offset, buffer, size);
    return size;
}

static int vesa_driver_ioctl(uint32_t command, void* arg) {
    // IOCTL komutları için
    switch (command) {
        case 0x1001: // Set mode
            return vesa_set_mode(*(uint16_t*)arg);
        case 0x1002: // Get mode info
            return vesa_get_mode_info(*(uint16_t*)arg, (vbe_mode_info_t*)arg);
        case 0x1003: // Put pixel
            {
                struct pixel_args {
                    int x, y;
                    rgba_t color;
                } *args = (struct pixel_args*)arg;
                return vesa_put_pixel(args->x, args->y, args->color);
            }
        case 0x1004: // Fill rect
            {
                struct rect_args {
                    int x, y, width, height;
                    rgba_t color;
                } *args = (struct rect_args*)arg;
                return vesa_fill_rect(args->x, args->y, args->width, args->height, args->color);
            }
    }
    return -1;
}

static int vesa_driver_shutdown(void) {
    printf("VESA/VBE sürücüsü kapatılıyor...\n");
    vesa_initialized = 0;
    return 0;
}

int vesa_init() {
    printf("VESA/VBE başlatılıyor...\n");
    
    // Controller info'yu al
    if (vesa_get_controller_info(&vesa_driver.controller_info) != 0) {
        printf("VESA controller info alınamadı\n");
        return -1;
    }
    
    // VESA imzasını kontrol et
    if (vesa_driver.controller_info.signature != VBE_CONTROLLER_SIGNATURE) {
        printf("VESA signature bulunamadı\n");
        return -1;
    }
    
    printf("VESA Version: %d.%d\n", 
           vesa_driver.controller_info.version >> 8, 
           vesa_driver.controller_info.version & 0xFF);
    
    printf("Total Memory: %d KB\n", vesa_driver.controller_info.total_memory * 64);
    
    // Varsayılan modu ayarla (1024x768x32)
    uint16_t best_mode = vesa_find_best_mode(1024, 768, 32);
    if (best_mode == 0xFFFF) {
        printf("Uygun video modu bulunamadı\n");
        return -1;
    }
    
    if (vesa_set_mode(best_mode) != 0) {
        printf("Video modu ayarlanamadı\n");
        return -1;
    }
    
    vesa_initialized = 1;
    return 0;
}

int vesa_get_controller_info(vbe_controller_info_t* info) {
    if (!info) return -1;
    
    // BIOS çağrısı yap
    uint16_t result = vbe_bios_call(VBE_GET_CONTROLLER_INFO, 0, 0, 0, 0, (uint16_t)info);
    
    if (result != 0x004F) {
        return -1;
    }
    
    return 0;
}

int vesa_get_mode_info(uint16_t mode, vbe_mode_info_t* info) {
    if (!info) return -1;
    
    // BIOS çağrısı yap
    uint16_t result = vbe_bios_call(VBE_GET_MODE_INFO, mode, 0, 0, 0, (uint16_t)info);
    
    if (result != 0x004F) {
        return -1;
    }
    
    return 0;
}

int vesa_set_mode(uint16_t mode) {
    // Mode info'yu al
    vbe_mode_info_t mode_info;
    if (vesa_get_mode_info(mode, &mode_info) != 0) {
        return -1;
    }
    
    // Mode'u ayarla
    uint16_t result = vbe_bios_call(VBE_SET_MODE, mode | 0x4000, 0, 0, 0, 0); // 0x4000 = linear framebuffer
    
    if (result != 0x004F) {
        return -1;
    }
    
    // Framebuffer adresini ayarla
    if (mode_info.framebuffer_ptr != 0) {
        vesa_driver.framebuffer = (uint8_t*)mode_info.framebuffer_ptr;
        vesa_driver.framebuffer_size = mode_info.bytes_per_scanline * mode_info.resolution_y;
        
        // Framebuffer'ı map et (gerçek sistemde memory mapping gerekir)
        printf("Framebuffer: 0x%08X, Boyut: %d bytes\n", 
               mode_info.framebuffer_ptr, vesa_driver.framebuffer_size);
    }
    
    vesa_driver.current_mode = mode;
    vesa_driver.current_mode_info = mode_info;
    
    printf("Video modu ayarlandı: %dx%dx%d\n", 
           mode_info.resolution_x, mode_info.resolution_y, 
           mode_info.red_mask_size + mode_info.green_mask_size + mode_info.blue_mask_size);
    
    return 0;
}

int vesa_get_current_mode(uint16_t* mode) {
    if (!mode) return -1;
    
    uint16_t result = vbe_bios_call(VBE_GET_CURRENT_MODE, 0, 0, 0, 0, 0);
    
    if (result != 0x004F) {
        return -1;
    }
    
    *mode = result; // AX register'ında mode değeri döner
    return 0;
}

int vesa_set_palette(uint8_t start, uint8_t count, rgba_t* palette) {
    if (!palette) return -1;
    
    // BIOS çağrısı yap
    uint16_t result = vbe_bios_call(VBE_SET_PALETTE_DATA, start, count, 0, 0, (uint16_t)palette);
    
    if (result != 0x004F) {
        return -1;
    }
    
    return 0;
}

int vesa_put_pixel(int x, int y, rgba_t color) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    
    // Sınırları kontrol et
    if (x < 0 || x >= mode->resolution_x || y < 0 || y >= mode->resolution_y) {
        return -1;
    }
    
    // Pixel formatına göre yaz
    uint8_t* pixel = vesa_driver.framebuffer + (y * mode->bytes_per_scanline) + (x * (mode->red_mask_size + mode->green_mask_size + mode->blue_mask_size) / 8);
    
    if (mode->memory_model == VBE_MEMORY_MODEL_DIRECT_COLOR) {
        // 32-bit color
        pixel[0] = color.b;
        pixel[1] = color.g;
        pixel[2] = color.r;
        pixel[3] = color.a;
    } else if (mode->memory_model == VBE_MEMORY_MODEL_256_COLOR) {
        // 8-bit paletli
        *pixel = (color.r >> 5) << 5 | (color.g >> 5) << 2 | (color.b >> 6);
    }
    
    return 0;
}

int vesa_get_pixel(int x, int y, rgba_t* color) {
    if (!vesa_initialized || !vesa_driver.framebuffer || !color) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    
    // Sınırları kontrol et
    if (x < 0 || x >= mode->resolution_x || y < 0 || y >= mode->resolution_y) {
        return -1;
    }
    
    // Pixel formatına göre oku
    uint8_t* pixel = vesa_driver.framebuffer + (y * mode->bytes_per_scanline) + (x * (mode->red_mask_size + mode->green_mask_size + mode->blue_mask_size) / 8);
    
    if (mode->memory_model == VBE_MEMORY_MODEL_DIRECT_COLOR) {
        // 32-bit color
        color->b = pixel[0];
        color->g = pixel[1];
        color->r = pixel[2];
        color->a = pixel[3];
    } else if (mode->memory_model == VBE_MEMORY_MODEL_256_COLOR) {
        // 8-bit paletli
        uint8_t index = *pixel;
        color->r = (index >> 5) * 255 / 7;
        color->g = ((index >> 2) & 0x07) * 255 / 7;
        color->b = (index & 0x03) * 255 / 3;
        color->a = 255;
    }
    
    return 0;
}

int vesa_fill_rect(int x, int y, int width, int height, rgba_t color) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    
    // Sınırları kontrol et
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > mode->resolution_x) width = mode->resolution_x - x;
    if (y + height > mode->resolution_y) height = mode->resolution_y - y;
    
    if (width <= 0 || height <= 0) return -1;
    
    // Rectangle'ı doldur
    for (int py = y; py < y + height; py++) {
        for (int px = x; px < x + width; px++) {
            vesa_put_pixel(px, py, color);
        }
    }
    
    return 0;
}

int vesa_draw_line(int x1, int y1, int x2, int y2, rgba_t color) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    // Bresenham line drawing algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        vesa_put_pixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    return 0;
}

int vesa_draw_char(int x, int y, char c, rgba_t fg_color, rgba_t bg_color) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    // 8x8 font kullan (basit implementasyon)
    extern uint8_t font_8x8[256][8]; // Font verisi
    
    if (c >= 32 && c <= 126) {
        for (int row = 0; row < 8; row++) {
            uint8_t font_row = font_8x8[c - 32][row];
            for (int col = 0; col < 8; col++) {
                rgba_t pixel_color = (font_row & (0x80 >> col)) ? fg_color : bg_color;
                vesa_put_pixel(x + col, y + row, pixel_color);
            }
        }
    }
    
    return 0;
}

int vesa_draw_string(int x, int y, const char* str, rgba_t fg_color, rgba_t bg_color) {
    if (!vesa_initialized || !vesa_driver.framebuffer || !str) return -1;
    
    int current_x = x;
    while (*str) {
        if (*str == '\n') {
            current_x = x;
            y += 8;
        } else {
            vesa_draw_char(current_x, y, *str, fg_color, bg_color);
            current_x += 8;
        }
        str++;
    }
    
    return 0;
}

int vesa_copy_rect(int src_x, int src_y, int dst_x, int dst_y, int width, int height) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    
    // Sınırları kontrol et
    if (src_x < 0 || src_y < 0 || dst_x < 0 || dst_y < 0) return -1;
    if (src_x + width > mode->resolution_x || src_y + height > mode->resolution_y) return -1;
    if (dst_x + width > mode->resolution_x || dst_y + height > mode->resolution_y) return -1;
    
    // Copy direction'u belirle (overlap kontrolü)
    int src_offset = src_y * mode->bytes_per_scanline + src_x * 4;
    int dst_offset = dst_y * mode->bytes_per_scanline + dst_x * 4;
    
    if (src_offset < dst_offset) {
        // Yukarıdan aşağıya kopyala
        for (int y = height - 1; y >= 0; y--) {
            uint8_t* src_line = vesa_driver.framebuffer + ((src_y + y) * mode->bytes_per_scanline) + (src_x * 4);
            uint8_t* dst_line = vesa_driver.framebuffer + ((dst_y + y) * mode->bytes_per_scanline) + (dst_x * 4);
            memcpy(dst_line, src_line, width * 4);
        }
    } else {
        // Aşağıdan yukarıya kopyala
        for (int y = 0; y < height; y++) {
            uint8_t* src_line = vesa_driver.framebuffer + ((src_y + y) * mode->bytes_per_scanline) + (src_x * 4);
            uint8_t* dst_line = vesa_driver.framebuffer + ((dst_y + y) * mode->bytes_per_scanline) + (dst_x * 4);
            memcpy(dst_line, src_line, width * 4);
        }
    }
    
    return 0;
}

int vesa_scroll_up(int lines) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    
    if (lines >= mode->resolution_y) {
        // Ekranı tamamen temizle
        rgba_t black = {0, 0, 0, 255};
        vesa_fill_rect(0, 0, mode->resolution_x, mode->resolution_y, black);
    } else {
        // Yukarı kaydır
        vesa_copy_rect(0, lines, 0, 0, mode->resolution_x, mode->resolution_y - lines);
        
        // Alt kısmı temizle
        rgba_t black = {0, 0, 0, 255};
        vesa_fill_rect(0, mode->resolution_y - lines, mode->resolution_x, lines, black);
    }
    
    return 0;
}

int vesa_clear_screen(rgba_t color) {
    if (!vesa_initialized || !vesa_driver.framebuffer) return -1;
    
    vbe_mode_info_t* mode = &vesa_driver.current_mode_info;
    return vesa_fill_rect(0, 0, mode->resolution_x, mode->resolution_y, color);
}

uint16_t vesa_find_best_mode(int width, int height, int bpp) {
    printf("En iyi video modu aranıyor: %dx%dx%d\n", width, height, bpp);
    
    // Mode listesini al
    uint16_t* mode_list = (uint16_t*)vesa_driver.controller_info.video_mode_ptr;
    
    uint16_t best_mode = 0xFFFF;
    int best_score = -1;
    
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        uint16_t mode = mode_list[i];
        vbe_mode_info_t mode_info;
        
        if (vesa_get_mode_info(mode, &mode_info) != 0) {
            continue;
        }
        
        // Mode'u kontrol et
        if (!(mode_info.mode_attributes & VBE_MODE_SUPPORTED)) {
            continue;
        }
        
        if (!(mode_info.mode_attributes & VBE_MODE_GRAPHICS_MODE)) {
            continue;
        }
        
        if (!(mode_info.mode_attributes & VBE_MODE_LINEAR_FB)) {
            continue;
        }
        
        // Skor hesapla
        int score = 0;
        
        // Resolution skoru
        if (mode_info.resolution_x == width) score += 100;
        else if (mode_info.resolution_x > width) score += 50;
        else score -= 100;
        
        if (mode_info.resolution_y == height) score += 100;
        else if (mode_info.resolution_y > height) score += 50;
        else score -= 100;
        
        // Color depth skoru
        int mode_bpp = mode_info.red_mask_size + mode_info.green_mask_size + mode_info.blue_mask_size;
        if (mode_bpp == bpp) score += 100;
        else if (mode_bpp > bpp) score += 50;
        else score -= 100;
        
        if (score > best_score) {
            best_score = score;
            best_mode = mode;
        }
    }
    
    if (best_mode != 0xFFFF) {
        vbe_mode_info_t best_mode_info;
        if (vesa_get_mode_info(best_mode, &best_mode_info) == 0) {
            printf("Bulunan en iyi mod: %d (%dx%dx%d)\n", 
                   best_mode, best_mode_info.resolution_x, 
                   best_mode_info.resolution_y,
                   best_mode_info.red_mask_size + best_mode_info.green_mask_size + best_mode_info.blue_mask_size);
        }
    }
    
    return best_mode;
}

int vesa_list_modes() {
    printf("Desteklenen Video Modları:\n");
    
    uint16_t* mode_list = (uint16_t*)vesa_driver.controller_info.video_mode_ptr;
    
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        uint16_t mode = mode_list[i];
        vbe_mode_info_t mode_info;
        
        if (vesa_get_mode_info(mode, &mode_info) != 0) {
            continue;
        }
        
        if (!(mode_info.mode_attributes & VBE_MODE_SUPPORTED)) {
            continue;
        }
        
        if (!(mode_info.mode_attributes & VBE_MODE_GRAPHICS_MODE)) {
            continue;
        }
        
        int bpp = mode_info.red_mask_size + mode_info.green_mask_size + mode_info.blue_mask_size;
        printf("  Mode %04X: %dx%dx%d %s\n", 
               mode, mode_info.resolution_x, mode_info.resolution_y, bpp,
               (mode_info.mode_attributes & VBE_MODE_LINEAR_FB) ? "Linear" : "Banked");
    }
    
    return 0;
}

driver_t* create_vesa_driver(pci_device_t* device) {
    if (vesa_initialized) {
        printf("VESA/VBE zaten başlatılmış\n");
        return &vesa_driver.base;
    }
    
    // Sürücü yapısını ayarla
    strcpy(vesa_driver.base.name, "VESA/VBE Graphics");
    vesa_driver.base.type = DRIVER_TYPE_DISPLAY;
    vesa_driver.base.init = vesa_driver_init;
    vesa_driver.base.read = vesa_driver_read;
    vesa_driver.base.write = vesa_driver_write;
    vesa_driver.base.ioctl = vesa_driver_ioctl;
    vesa_driver.base.shutdown = vesa_driver_shutdown;
    
    printf("VESA/VBE sürücüsü oluşturuldu\n");
    return &vesa_driver.base;
}
