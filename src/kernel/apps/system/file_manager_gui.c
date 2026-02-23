#include "file_manager_gui.h"
#include "file_manager.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "printf.h"
#include "fs.h"
#include "string.h"
#include "memory.h"
#include "icons.h"
#include "mouse.h"
#include "ata.h"

// Zamanlayıcı için extern
extern uint32_t get_tick_count();

// Global değişkenler
static fm_data_t* fm_data = NULL;
static int fm_initialized = 0;

// İkon verileri (16x16)
static uint8_t icon_folder_16x16[] = { 0xFF, 0x00, 0x55, 0xAA /* ... sadeleştirilmiş ... */ };
static uint8_t icon_file_16x16[]   = { 0xAA, 0x55, 0xFF, 0x00 /* ... sadeleştirilmiş ... */ };

void fm_init() {
    if (fm_initialized) return;
    fm_data = (fm_data_t*)kmalloc(sizeof(fm_data_t));
    if (!fm_data) return;
    
    memset(fm_data, 0, sizeof(fm_data_t));
    strcpy(fm_data->current_path, "/");
    fm_initialized = 1;
    printf("Grafiksel Dosya Yoneticisi baslatildi\n");
}

void fm_refresh() {
    if (!fm_initialized || !fm_data) return;
    
    uint8_t* buffer = kmalloc(512);
    if (!buffer) return;
    
    // Sektör 100'den dizin okuma (GümüşOS standart sektörü)
    ata_read_sectors((uint32_t)buffer, 100, 1);
    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;
    
    fm_data->file_count = 0;

    // Klasörleri ve dosyaları ayıkla
    for (int i = 0; i < 16 && fm_data->file_count < FM_MAX_FILES; i++) {
        if (entry[i].name[0] == 0) break;
        if (entry[i].name[0] == 0xE5) continue;

        fm_file_info_t* file = &fm_data->files[fm_data->file_count];
        
        // İsim kopyalama
        memcpy(file->name, entry[i].name, 8);
        file->name[8] = '\0';
        
        file->size = entry[i].file_size;
        file->is_dir = (entry[i].attributes & 0x10) ? 1 : 0;
        file->is_selected = 0;
        
        fm_data->file_count++;
    }
    kfree(buffer);
}

void fm_draw_file_item(int x, int y, fm_file_info_t* file, int selected) {
    if (selected) {
        vga_draw_rect(x - 2, y - 2, 40, 40, FM_SELECTED_COLOR);
    }
    
    // İkon çizimi
    if (file->is_dir) vga_draw_icon(x, y, icon_folder_16x16, 16, 16);
    else vga_draw_icon(x, y, icon_file_16x16, 16, 16);
    
    // Dosya ismi (strrchr kullanımı burada kritik)
    char* display_name = file->name;
    vga_draw_text(x, y + 20, display_name, selected ? BLACK : WHITE);
}

void fm_draw_window(window_t* win) {
    if (!fm_initialized || !fm_data) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    vga_draw_rect(px + 1, py + 1, win->w * 8 - 2, win->h * 8 - 2, FM_BG_COLOR);

    // Dosyaları grid şeklinde diz
    for (int i = 0; i < fm_data->file_count; i++) {
        int ix = px + 10 + (i % 5) * 60;
        int iy = py + 45 + (i / 5) * 50;
        fm_draw_file_item(ix, iy, &fm_data->files[i], fm_data->files[i].is_selected);
    }
    
    // Üst bar (Path)
    vga_draw_text(px + 10, py + 10, fm_data->current_path, WHITE);
}

void fm_update(window_t* win) {
    mouse_state_t ms;
    get_mouse_state(&ms); // Fare durumunu güncelle
    fm_data->mouse_x = ms.x;
    fm_data->mouse_y = ms.y;
    fm_data->mouse_pressed = ms.buttons & 0x01;
}

void launch_file_manager_gui() {
    fm_init();
    fm_refresh();
    
    int win_id = create_window("Dosya Yoneticisi", 2, 2, 70, 25, (CYAN << 4) | BLACK);
    window_t* win = get_window(win_id);
    
    if (win) {
        win->draw_content = fm_draw_window;
        win->on_update = fm_update;
        win->data = fm_data;
    }
}