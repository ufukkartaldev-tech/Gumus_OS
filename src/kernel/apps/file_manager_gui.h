#ifndef FILE_MANAGER_GUI_H
#define FILE_MANAGER_GUI_H

#include <stdint.h>
#include "../core/window.h"
#include "../drivers/vga_gfx.h"

// Dosya Yöneticisi Sabitleri
#define FM_MAX_FILES 64
#define FM_FILES_PER_ROW 6
#define FM_ICON_SIZE 32
#define FM_ICON_SPACING 8
#define FM_TEXT_HEIGHT 12
#define FM_ITEM_HEIGHT (FM_ICON_SIZE + FM_TEXT_HEIGHT + 4)

// Dosya Yöneticisi Renkleri
#define FM_BG_COLOR 15           // Beyaz
#define FM_SELECTED_COLOR 4      // Kırmızı
#define FM_TEXT_COLOR 0          // Siyah
#define FM_BORDER_COLOR 8        // Gri
#define FM_PATH_BG_COLOR 7       // Açık gri

// Dosya Bilgisi
typedef struct {
    char name[32];
    uint32_t size;
    uint8_t is_dir;
    uint8_t is_selected;
    uint32_t modified_time;
} fm_file_info_t;

// Dosya Yöneticisi Pencere Verisi
typedef struct {
    fm_file_info_t files[FM_MAX_FILES];
    int file_count;
    int selected_index;
    int scroll_offset;
    char current_path[256];
    int view_mode; // 0: Icons, 1: List, 2: Details
    int sort_mode; // 0: Name, 1: Size, 2: Date
    int show_hidden;
    
    // Mouse durumu
    int mouse_x, mouse_y;
    int mouse_pressed;
    uint32_t last_click_time;
    int last_click_index;
    
    // Context menu
    int show_context_menu;
    int context_x, context_y;
} fm_data_t;

// Dosya Yöneticisi Fonksiyonları
void fm_init();
void fm_refresh();
void fm_draw_window(window_t* win);
void fm_handle_click(window_t* win, int x, int y);
void fm_handle_key(window_t* win, char c);
void fm_handle_mouse(window_t* win, int x, int y, int pressed);
void fm_update(window_t* win);

// Dosya işlemleri
void fm_open_file(int index);
void fm_select_file(int index);
void fm_toggle_selection(int index);
void fm_copy_selected();
void fm_move_selected();
void fm_delete_selected();
void fm_rename_file(int index);
void fm_create_folder();
void fm_navigate_up();
void fm_navigate_into(int index);

// UI yardımcı fonksiyonları
void fm_draw_icon(int x, int y, int is_dir, const char* ext);
void fm_draw_file_item(int x, int y, fm_file_info_t* file, int selected);
int fm_get_item_at_position(int x, int y, int item_width, int item_height);
void fm_draw_path_bar(window_t* win);
void fm_draw_toolbar(window_t* win);
void fm_draw_context_menu(window_t* win);

// Sıralama ve filtreleme
void fm_sort_files();
int fm_compare_files(const void* a, const void* b);
void fm_filter_files();

#endif
