#ifndef VESA_VBE_H
#define VESA_VBE_H

#include <stdint.h>
#include "driver.h"
#include "hardware_detect.h"

// VESA/VBE Sabitleri
#define VBE_CONTROLLER_SIGNATURE 0x41545356  // "VESA"
#define VBE_MODE_SIGNATURE       0x4154534D  // "MESA"

// VBE Fonksiyonları
#define VBE_GET_CONTROLLER_INFO  0x4F00
#define VBE_GET_MODE_INFO        0x4F01
#define VBE_SET_MODE             0x4F02
#define VBE_GET_CURRENT_MODE     0x4F03
#define VBE_SAVE_RESTORE_STATE   0x4F04
#define VBE_GET_WINDOW_CTRL      0x4F05
#define VBE_SET_WINDOW           0x4F06
#define VBE_GET_SCANLINE_LENGTH  0x4F07
#define VBE_GET_MAX_PIXEL_CLOCK  0x4F08
#define VBE_GET_PALETTE_FORMAT   0x4F09
#define VBE_SET_PALETTE_DATA     0x4F0A
#define VBE_GET_PROTECTED_MODE_INTERFACE 0x4F0B

// VBE Mode Attributes
#define VBE_MODE_SUPPORTED       0x0001
#define VBE_MODE_EXTENDED_INFO   0x0002
#define VBE_MODE_COLOR_MODE      0x0004
#define VBE_MODE_GRAPHICS_MODE   0x0008
#define VBE_MODE_VGA_COMPATIBLE  0x0010
#define VBE_MODE_NO_VGA_COMPAT   0x0020
#define VBE_MODE_LINEAR_FB       0x0080
#define VBE_MODE_DOUBLE_SCAN     0x0100
#define VBE_MODE_INTERLACED      0x0200
#define VBE_MODE_HARDWARE_TRIPLE 0x0400
#define VBE_MODE_NOT_HARWARE_TRIPLE 0x0800
#define VBE_MODE_STEREO          0x1000
#define VBE_MODE_DUAL_DISPLAY    0x2000

// VBE Memory Model
#define VBE_MEMORY_MODEL_TEXT    0x00
#define VBE_MEMORY_MODEL_CGA     0x01
#define VBE_MEMORY_MODEL_HERCULES 0x02
#define VBE_MEMORY_MODEL_PLANAR  0x03
#define VBE_MEMORY_MODEL_PACKED_PIXEL 0x04
#define VBE_MEMORY_MODEL_256_COLOR 0x05
#define VBE_MEMORY_MODEL_DIRECT_COLOR 0x06
#define VBE_MEMORY_MODEL_YUV     0x07

// VBE Controller Info Structure
typedef struct {
    uint16_t signature;           // "VESA"
    uint16_t version;             // VBE version
    uint32_t oem_string_ptr;      // Pointer to OEM string
    uint32_t capabilities;        // Capabilities flags
    uint32_t video_mode_ptr;       // Pointer to supported modes
    uint16_t total_memory;        // Total memory in 64KB blocks
    uint16_t oem_software_rev;    // OEM software revision
    uint32_t oem_vendor_name_ptr; // OEM vendor name
    uint32_t oem_product_name_ptr; // OEM product name
    uint32_t oem_product_rev_ptr; // OEM product revision
    uint8_t  reserved[222];       // Reserved
    uint8_t  oem_data[256];      // OEM data
} __attribute__((packed)) vbe_controller_info_t;

// VBE Mode Info Structure
typedef struct {
    uint16_t mode_attributes;     // Mode attributes
    uint8_t  window_a_attributes; // Window A attributes
    uint8_t  window_b_attributes; // Window B attributes
    uint16_t window_granularity;  // Window granularity
    uint16_t window_size;         // Window size
    uint16_t window_a_start;      // Window A start segment
    uint16_t window_b_start;      // Window B start segment
    uint32_t window_position_ptr; // Window position pointer
    uint16_t bytes_per_scanline;  // Bytes per scanline
    uint16_t resolution_x;        // Horizontal resolution
    uint16_t resolution_y;        // Vertical resolution
    uint8_t  character_width;     // Character width
    uint8_t  character_height;    // Character height
    uint8_t  memory_model;        // Memory model
    uint8_t  bank_size;           // Bank size
    uint8_t  number_of_image_pages; // Number of image pages
    uint8_t  reserved1;           // Reserved
    uint8_t  red_mask_size;       // Red mask size
    uint8_t  red_field_position;  // Red field position
    uint8_t  green_mask_size;     // Green mask size
    uint8_t  green_field_position; // Green field position
    uint8_t  blue_mask_size;      // Blue mask size
    uint8_t  blue_field_position; // Blue field position
    uint8_t  reserved_mask_size;  // Reserved mask size
    uint8_t  reserved_field_position; // Reserved field position
    uint8_t  direct_color_attributes; // Direct color attributes
    uint32_t framebuffer_ptr;     // Framebuffer physical address
    uint32_t offscreen_memory_ptr; // Offscreen memory pointer
    uint16_t offscreen_memory_size; // Offscreen memory size
    uint16_t reserved2;           // Reserved
    uint8_t  reserved3[206];      // Reserved
} __attribute__((packed)) vbe_mode_info_t;

// VBE Protected Mode Interface
typedef struct {
    uint32_t set_window;          // Set window function
    uint32_t set_display_start;   // Set display start function
    uint32_t set_palette;        // Set palette function
    uint32_t interface_length;    // Interface length
    uint32_t interface_version;   // Interface version
    uint32_t oem_string;         // OEM string
    uint32_t reserved[4];        // Reserved
    uint8_t  reserved2[256];      // Reserved
} __attribute__((packed)) vbe_pm_interface_t;

// VESA Driver Structure
typedef struct {
    driver_t base;
    vbe_controller_info_t controller_info;
    vbe_mode_info_t current_mode_info;
    vbe_pm_interface_t pm_interface;
    uint16_t current_mode;
    uint8_t* framebuffer;
    uint32_t framebuffer_size;
    int initialized;
} vesa_driver_t;

// Common Video Modes
#define VBE_MODE_640x480x8    0x101
#define VBE_MODE_640x480x16   0x111
#define VBE_MODE_640x480x24   0x112
#define VBE_MODE_640x480x32   0x115
#define VBE_MODE_800x600x8    0x103
#define VBE_MODE_800x600x16   0x114
#define VBE_MODE_800x600x24   0x115
#define VBE_MODE_800x600x32   0x118
#define VBE_MODE_1024x768x8   0x105
#define VBE_MODE_1024x768x16  0x117
#define VBE_MODE_1024x768x24  0x118
#define VBE_MODE_1024x768x32  0x11B
#define VBE_MODE_1280x1024x8  0x107
#define VBE_MODE_1280x1024x16 0x11A
#define VBE_MODE_1280x1024x24 0x11B
#define VBE_MODE_1280x1024x32 0x11E

// Color Structure
typedef struct {
    uint8_t r, g, b, a;
} rgba_t;

// VESA Fonksiyonları
int vesa_init();
int vesa_get_controller_info(vbe_controller_info_t* info);
int vesa_get_mode_info(uint16_t mode, vbe_mode_info_t* info);
int vesa_set_mode(uint16_t mode);
int vesa_get_current_mode(uint16_t* mode);
int vesa_set_palette(uint8_t start, uint8_t count, rgba_t* palette);
int vesa_put_pixel(int x, int y, rgba_t color);
int vesa_get_pixel(int x, int y, rgba_t* color);
int vesa_fill_rect(int x, int y, int width, int height, rgba_t color);
int vesa_draw_line(int x1, int y1, int x2, int y2, rgba_t color);
int vesa_draw_char(int x, int y, char c, rgba_t fg_color, rgba_t bg_color);
int vesa_draw_string(int x, int y, const char* str, rgba_t fg_color, rgba_t bg_color);
int vesa_copy_rect(int src_x, int src_y, int dst_x, int dst_y, int width, int height);
int vesa_scroll_up(int lines);
int vesa_clear_screen(rgba_t color);

// Mode Seçim Fonksiyonları
uint16_t vesa_find_best_mode(int width, int height, int bpp);
int vesa_list_modes();
int vesa_get_mode_info_string(uint16_t mode, char* buffer);

// Sürücü Oluşturma Fonksiyonu
driver_t* create_vesa_driver(pci_device_t* device);

#endif
