#include "file_manager_gui.h"
#include "file_manager.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "fs.h"
#include "string.h"
#include "memory.h"
#include "icons.h"
#include "image_viewer.h"
#include "../drivers/mouse.h"

// Global değişkenler
static fm_data_t* fm_data = NULL;
static int fm_initialized = 0;

// Basit ikon verileri (16x16 piksel)
static uint8_t icon_folder_16x16[] = {
    0x00,0x00,0x7E,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0x7E,0x00,0x00,0x00,
    0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00
};

static uint8_t icon_file_16x16[] = {
    0x00,0x00,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFE,0x00,0x00,0x00,
    0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00
};

static uint8_t icon_image_16x16[] = {
    0x00,0x00,0xFE,0x81,0xBD,0xBD,0xBD,0xBD,
    0xBD,0xBD,0xBD,0xBD,0xFE,0x00,0x00,0x00,
    0x00,0x00,0xFF,0x81,0xBD,0xBD,0xBD,0xBD,
    0xBD,0xBD,0xBD,0xBD,0xFF,0x00,0x00,0x00
};

void fm_init() {
    if (fm_initialized) return;
    
    fm_data = (fm_data_t*)kmalloc(sizeof(fm_data_t));
    if (!fm_data) return;
    
    memset(fm_data, 0, sizeof(fm_data_t));
    strcpy(fm_data->current_path, "/");
    fm_data->view_mode = 0; // Icon view
    fm_data->sort_mode = 0; // By name
    fm_data->show_hidden = 0;
    
    fm_initialized = 1;
    printf("Grafiksel Dosya Yöneticisi başlatıldı\n");
}

void fm_refresh() {
    if (!fm_initialized || !fm_data) return;
    
    // Dosya listesini oku
    uint8_t* buffer = kmalloc(512);
    if (!buffer) return;
    
    ata_read_sectors((uint32_t)buffer, 100, 1);
    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;
    
    fm_data->file_count = 0;
    
    // ".." (üst dizin) ekle
    if (strcmp(fm_data->current_path, "/") != 0) {
        strcpy(fm_data->files[fm_data->file_count].name, "..");
        fm_data->files[fm_data->file_count].is_dir = 1;
        fm_data->files[fm_data->file_count].size = 0;
        fm_data->files[fm_data->file_count].is_selected = 0;
        fm_data->file_count++;
    }
    
    // "." (mevcut dizin) ekle
    strcpy(fm_data->files[fm_data->file_count].name, ".");
    fm_data->files[fm_data->file_count].is_dir = 1;
    fm_data->files[fm_data->file_count].size = 0;
    fm_data->files[fm_data->file_count].is_selected = 0;
    fm_data->file_count++;
    
    // Dosyaları oku
    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t) && fm_data->file_count < FM_MAX_FILES; i++) {
        if (entry[i].name[0] == 0) break;
        if (entry[i].name[0] == 0xE5) continue;
        if (entry[i].attributes & 0x02) continue; // Hidden file
        
        // Adı ayrıştır
        int k = 0;
        for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) {
            fm_data->files[fm_data->file_count].name[k++] = entry[i].name[j];
        }
        if (entry[i].ext[0] != ' ') {
            fm_data->files[fm_data->file_count].name[k++] = '.';
            for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) {
                fm_data->files[fm_data->file_count].name[k++] = entry[i].ext[j];
            }
        }
        fm_data->files[fm_data->file_count].name[k] = '\0';
        
        fm_data->files[fm_data->file_count].size = entry[i].file_size;
        fm_data->files[fm_data->file_count].is_dir = (entry[i].attributes & 0x10);
        fm_data->files[fm_data->file_count].is_selected = 0;
        fm_data->files[fm_data->file_count].modified_time = entry[i].write_time;
        
        fm_data->file_count++;
    }
    
    kfree(buffer);
    
    // Dosyaları sırala
    fm_sort_files();
    
    printf("Dosya listesi yenilendi: %d dosya bulundu\n", fm_data->file_count);
}

void fm_sort_files() {
    if (!fm_data) return;
    
    // Basit bubble sort - klasörler önce, sonra dosyalar
    for (int i = 0; i < fm_data->file_count - 1; i++) {
        for (int j = 0; j < fm_data->file_count - i - 1; j++) {
            int swap = 0;
            
            if (fm_data->files[j].is_dir && !fm_data->files[j+1].is_dir) {
                swap = 1; // Klasör her önce gelir
            } else if (fm_data->files[j].is_dir == fm_data->files[j+1].is_dir) {
                // Aynı türse isme göre sırala
                if (strcmp(fm_data->files[j].name, fm_data->files[j+1].name) > 0) {
                    swap = 1;
                }
            }
            
            if (swap) {
                fm_file_info_t temp = fm_data->files[j];
                fm_data->files[j] = fm_data->files[j+1];
                fm_data->files[j+1] = temp;
            }
        }
    }
}

void fm_draw_icon(int x, int y, int is_dir, const char* ext) {
    if (is_dir) {
        vga_draw_icon(x, y, icon_folder_16x16, 16, 16);
    } else if (ext && (strcmp(ext, "BMP") == 0 || strcmp(ext, "bmp") == 0)) {
        vga_draw_icon(x, y, icon_image_16x16, 16, 16);
    } else {
        vga_draw_icon(x, y, icon_file_16x16, 16, 16);
    }
}

void fm_draw_file_item(int x, int y, fm_file_info_t* file, int selected) {
    // Arka plan
    if (selected) {
        vga_draw_rect(x - 2, y - 2, FM_ICON_SIZE + 4, FM_ITEM_HEIGHT + 4, FM_SELECTED_COLOR);
    }
    
    // İkon
    char* ext = NULL;
    if (!file->is_dir) {
        ext = strrchr(file->name, '.');
        if (ext) ext++;
    }
    fm_draw_icon(x, y, file->is_dir, ext);
    
    // Dosya adı
    int text_x = x + (FM_ICON_SIZE - strlen(file->name) * 8) / 2;
    int text_y = y + FM_ICON_SIZE + 2;
    
    if (selected) {
        vga_draw_text(text_x, text_y, file->name, FM_BG_COLOR);
    } else {
        vga_draw_text(text_x, text_y, file->name, FM_TEXT_COLOR);
    }
}

void fm_draw_path_bar(window_t* win) {
    if (!fm_data) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    
    // Path bar arka plan
    vga_draw_rect(px + 2, py + 2, width - 4, 16, FM_PATH_BG_COLOR);
    
    // Path text
    vga_draw_text(px + 4, py + 6, fm_data->current_path, FM_TEXT_COLOR);
}

void fm_draw_toolbar(window_t* win) {
    if (!fm_data) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    
    // Toolbar arka plan
    vga_draw_rect(px + 2, py + 20, width - 4, 20, FM_PATH_BG_COLOR);
    
    // Basit toolbar butonları
    vga_draw_text(px + 4, py + 26, "[YUKARI] [YENI] [KOPYALA] [SIL]", FM_TEXT_COLOR);
}

void fm_draw_window(window_t* win) {
    if (!fm_initialized || !fm_data) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    int height = win->h * 8;
    
    // Ana arka plan
    vga_draw_rect(px + 1, py + 1, width - 2, height - 2, FM_BG_COLOR);
    
    // Path bar ve toolbar
    fm_draw_path_bar(win);
    fm_draw_toolbar(win);
    
    // Dosya alanı
    int file_area_y = py + 42;
    int file_area_height = height - 44;
    
    // Grid layout için hesapla
    int items_per_row = (width - 20) / (FM_ICON_SIZE + FM_ICON_SPACING);
    if (items_per_row < 1) items_per_row = 1;
    
    int start_index = fm_data->scroll_offset;
    int max_visible = (file_area_height / FM_ITEM_HEIGHT) * items_per_row;
    
    // Dosyaları çiz
    for (int i = start_index; i < fm_data->file_count && i < start_index + max_visible; i++) {
        int row = (i - start_index) / items_per_row;
        int col = (i - start_index) % items_per_row;
        
        int x = px + 10 + col * (FM_ICON_SIZE + FM_ICON_SPACING);
        int y = file_area_y + 4 + row * FM_ITEM_HEIGHT;
        
        fm_draw_file_item(x, y, &fm_data->files[i], fm_data->files[i].is_selected);
    }
    
    // Context menu
    if (fm_data->show_context_menu) {
        fm_draw_context_menu(win);
    }
}

void fm_draw_context_menu(window_t* win) {
    if (!fm_data) return;
    
    int menu_x = fm_data->context_x;
    int menu_y = fm_data->context_y;
    int menu_width = 120;
    int menu_height = 100;
    
    // Menu arka plan
    vga_draw_rect(menu_x, menu_y, menu_width, menu_height, FM_BG_COLOR);
    vga_draw_rect(menu_x, menu_y, menu_width, menu_height, FM_BORDER_COLOR);
    
    // Menu öğeleri
    vga_draw_text(menu_x + 4, menu_y + 4, "Aç", FM_TEXT_COLOR);
    vga_draw_text(menu_x + 4, menu_y + 20, "Kopyala", FM_TEXT_COLOR);
    vga_draw_text(menu_x + 4, menu_y + 36, "Taşı", FM_TEXT_COLOR);
    vga_draw_text(menu_x + 4, menu_y + 52, "Sil", FM_TEXT_COLOR);
    vga_draw_text(menu_x + 4, menu_y + 68, "Yeniden Adlandır", FM_TEXT_COLOR);
    vga_draw_text(menu_x + 4, menu_y + 84, "Özellikler", FM_TEXT_COLOR);
}

int fm_get_item_at_position(int x, int y, int item_width, int item_height) {
    if (!fm_data) return -1;
    
    int items_per_row = item_width / (FM_ICON_SIZE + FM_ICON_SPACING);
    if (items_per_row < 1) items_per_row = 1;
    
    int col = x / (FM_ICON_SIZE + FM_ICON_SPACING);
    int row = y / item_height;
    
    int index = fm_data->scroll_offset + row * items_per_row + col;
    
    if (index >= 0 && index < fm_data->file_count) {
        return index;
    }
    
    return -1;
}

void fm_select_file(int index) {
    if (!fm_data || index < 0 || index >= fm_data->file_count) return;
    
    // Tüm seçimleri temizle
    for (int i = 0; i < fm_data->file_count; i++) {
        fm_data->files[i].is_selected = 0;
    }
    
    fm_data->files[index].is_selected = 1;
    fm_data->selected_index = index;
}

void fm_toggle_selection(int index) {
    if (!fm_data || index < 0 || index >= fm_data->file_count) return;
    
    fm_data->files[index].is_selected = !fm_data->files[index].is_selected;
    if (fm_data->files[index].is_selected) {
        fm_data->selected_index = index;
    }
}

void fm_open_file(int index) {
    if (!fm_data || index < 0 || index >= fm_data->file_count) return;
    
    fm_file_info_t* file = &fm_data->files[index];
    
    if (file->is_dir) {
        if (strcmp(file->name, "..") == 0) {
            fm_navigate_up();
        } else if (strcmp(file->name, ".") != 0) {
            fm_navigate_into(index);
        }
    } else {
        // Dosyayı aç
        printf("Dosya açılıyor: %s\n", file->name);
        
        // Eğer resim dosyası ise image viewer'ı aç
        char* ext = strrchr(file->name, '.');
        if (ext && (strcmp(ext, ".BMP") == 0 || strcmp(ext, ".bmp") == 0)) {
            init_image_viewer(file->name);
        } else {
            // Metin editörü aç
            create_window(file->name, 10, 5, 60, 20, (BLUE << 4) | WHITE);
        }
    }
}

void fm_navigate_up() {
    if (!fm_data) return;
    
    // Basit implementasyon - root'a git
    strcpy(fm_data->current_path, "/");
    fm_refresh();
}

void fm_navigate_into(int index) {
    if (!fm_data || index < 0 || index >= fm_data->file_count) return;
    
    fm_file_info_t* file = &fm_data->files[index];
    if (!file->is_dir) return;
    
    // Basit implementasyon - path'i güncelle
    if (strcmp(fm_data->current_path, "/") == 0) {
        sprintf(fm_data->current_path, "/%s", file->name);
    } else {
        strcat(fm_data->current_path, "/");
        strcat(fm_data->current_path, file->name);
    }
    
    fm_refresh();
}

void fm_handle_click(window_t* win, int x, int y) {
    if (!fm_data) return;
    
    // Pencere koordinatlarını dönüştür
    int px = x - win->x * 8;
    int py = y - win->y * 8;
    
    // Context menu kontrolü
    if (fm_data->show_context_menu) {
        fm_data->show_context_menu = 0;
        return;
    }
    
    // Toolbar kontrolü
    if (py >= 20 && py < 40) {
        if (px < 60) {
            fm_navigate_up();
        } else if (px < 100) {
            fm_create_folder();
        } else if (px < 160) {
            fm_copy_selected();
        } else if (px < 200) {
            fm_delete_selected();
        }
        return;
    }
    
    // Dosya alanı kontrolü
    if (py >= 42) {
        int file_x = px - 10;
        int file_y = py - 46;
        
        int index = fm_get_item_at_position(file_x, file_y, win->w * 8, FM_ITEM_HEIGHT);
        
        if (index >= 0) {
            uint32_t current_time = 0; // Timer'dan alınmalı
            
            // Çift tıklama kontrolü
            if (index == fm_data->last_click_index && 
                (current_time - fm_data->last_click_time) < 500) { // 500ms içinde çift tık
                fm_open_file(index);
            } else {
                // Tek tıklama - seç
                if (current_time - fm_data->last_click_time > 500) {
                    fm_select_file(index);
                } else {
                    fm_toggle_selection(index);
                }
            }
            
            fm_data->last_click_index = index;
            fm_data->last_click_time = current_time;
        }
    }
}

void fm_handle_mouse(window_t* win, int x, int y, int pressed) {
    if (!fm_data) return;
    
    fm_data->mouse_x = x;
    fm_data->mouse_y = y;
    fm_data->mouse_pressed = pressed;
    
    if (pressed) {
        fm_handle_click(win, x, y);
    }
}

void fm_copy_selected() {
    if (!fm_data) return;
    
    int selected_count = 0;
    for (int i = 0; i < fm_data->file_count; i++) {
        if (fm_data->files[i].is_selected) {
            selected_count++;
            printf("Kopyalanıyor: %s\n", fm_data->files[i].name);
        }
    }
    
    if (selected_count == 0) {
        printf("Seçili dosya yok\n");
    }
}

void fm_delete_selected() {
    if (!fm_data) return;
    
    int selected_count = 0;
    for (int i = 0; i < fm_data->file_count; i++) {
        if (fm_data->files[i].is_selected) {
            selected_count++;
            printf("Siliniyor: %s\n", fm_data->files[i].name);
        }
    }
    
    if (selected_count == 0) {
        printf("Seçili dosya yok\n");
    } else {
        fm_refresh(); // Listeyi yenile
    }
}

void fm_create_folder() {
    if (!fm_data) return;
    
    printf("Yeni klasör oluşturuluyor...\n");
    // Gerçek implementasyon dosya sistemi çağrıları gerektirir
    fm_refresh();
}

void fm_update(window_t* win) {
    // Mouse pozisyonunu güncelle
    mouse_state_t mouse_state;
    if (get_mouse_state(&mouse_state) == 0) {
        fm_data->mouse_x = mouse_state.x;
        fm_data->mouse_y = mouse_state.y;
        fm_data->mouse_pressed = mouse_state.buttons & 0x01;
    }
}

// Ana giriş fonksiyonu
void launch_file_manager_gui() {
    fm_init();
    fm_refresh();
    
    int win_id = create_window("Dosya Yöneticisi", 2, 2, 70, 25, (CYAN << 4) | BLACK);
    window_t* win = get_window(win_id);
    
    if (win) {
        win->draw_content = fm_draw_window;
        win->on_click = fm_handle_click;
        win->on_update = fm_update;
        win->data = fm_data;
    }
}
