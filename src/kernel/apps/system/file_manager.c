#include "file_manager.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "fs.h"
#include "string.h"
#include "memory.h"
#include "icons.h"
#include "image_viewer.h"

// Basit Dosya Yöneticisi ve Metin Editörü

// Dosya Listesi (fs_ls'in bellek versiyonu)
// Bu fonksiyon fs.c'ye eklenmeliydi ama şimdilik burada manuel okuyacağız.
// fs.c'deki yapılar statik/gizli değilse erişebiliriz.

#define MAX_FILES 16
typedef struct {
    char name[12];
    int size;
    int is_dir;
} file_info_t;

file_info_t file_list[MAX_FILES];
int file_count = 0;

// Çift Tıklama Takibi
static uint32_t fm_last_click_time = 0;
static int fm_last_click_index = -1;

void refresh_file_list() {
    // fs_read_dir gibi bir fonksiyon olmadığı için fs_ls mantığını kopyalıyoruz
    // Root Sector: 100
    uint8_t* buffer = kmalloc(512);
    ata_read_sectors((uint32_t)buffer, 100, 1);
    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;
    
    file_count = 0;
    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
        if (entry[i].name[0] == 0) break;
        if (entry[i].name[0] == 0xE5) continue;
        
        // Adı ayrıştır
        int k = 0;
        for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) file_list[file_count].name[k++] = entry[i].name[j];
        if (entry[i].ext[0] != ' ') {
            file_list[file_count].name[k++] = '.';
            for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) file_list[file_count].name[k++] = entry[i].ext[j];
        }
        file_list[file_count].name[k] = '\0';
        file_list[file_count].size = entry[i].file_size;
        file_list[file_count].is_dir = (entry[i].attributes & 0x10);
        
        file_count++;
        if (file_count >= MAX_FILES) break;
    }
    kfree(buffer);
}

void file_manager_draw(window_t* win) {
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Arkaplan (Beyaz)
    vga_draw_rect(px + 1, py + 10, (win->w * 8) - 2, (win->h * 8) - 11, 15);
    
    // Dosyaları Listele
    for (int i = 0; i < file_count; i++) {
        int fy = py + 12 + (i * 14); // Satır yüksekliği arttı (10 -> 14)
        
        // İkon (Dosya mı Klasör mü?)
        if (file_list[i].is_dir) {
            vga_draw_icon(px + 4, fy, icon_folder_12x12, 12, 12);
        } else {
            vga_draw_icon(px + 4, fy, icon_file_12x12, 12, 12);
        }
        
        // İsim (Siyah)
        vga_draw_text(px + 20, fy + 2, file_list[i].name, 0);
    }
}

// Stray braces removed


// Text Editor Callbackleri
void text_editor_draw(window_t* win) {
    if (!win->data) return;
    
    char* content = (char*)win->data;
    int px = win->x * 8;
    int py = win->y * 8;
    
    // İçerik Alanı Arkaplanı (Beyaz)
    vga_draw_rect(px + 2, py + 10, (win->w * 8) - 4, (win->h * 8) - 12, 15);
    
    // Metni Çiz (Basit word-wrap olmadan)
    // Şimdilik sadece ilk 10 satırı gösterelim
    int line = 0;
    int col = 0;
    int max_chars_per_line = win->w - 1;
    
    int content_len = strlen(content);
    int draw_x = px + 4;
    int draw_y = py + 12;
    
    for(int i = 0; i < content_len; i++) {
        char c = content[i];
        if (c == '\n') {
            line++;
            col = 0;
            draw_y += 8;
            draw_x = px + 4;
        } else {
            vga_draw_char(draw_x, draw_y, c, 0); // Siyah yazı
            draw_x += 8;
            col++;
            if (col >= max_chars_per_line) {
                line++;
                col = 0;
                draw_y += 8;
                draw_x = px + 4;
            }
        }
        if (line > win->h - 3) break; // Pencere taştı
    }
}

void text_editor_close(window_t* win) {
    if (win->data) {
        kfree(win->data); // Belleği serbest bırak
        win->data = 0;
    }
}

// Dosya Aç (Text Editor)
void open_text_editor(const char* filename) {
    char* content = fs_read(filename);
    if (!content) return;
    
    // Pencereyi oluştur
    int win_id = create_window(filename, 10, 5, 30, 20, (4 << 4) | 15); // Kırmızı Çerçeve
    
    // Callbackleri ve Veriyi Ata
    set_window_callbacks(win_id, text_editor_draw, 0); // Tıklama işlevi yok şimdilik
    set_window_data(win_id, content, text_editor_close);
}

void file_manager_click(window_t* win, int x, int y) {
    // x, y pencere içi koordinatlar (piksel)
    // Başlık çubuğu ~8 piksel, içerik başlangıcı ~12. piksel
    
    if (y < 12) return; // Başlık çubuğu vb.
    
    int index = (y - 12) / 14; 
    if (index >= 0 && index < file_count) {
        
        // Çift Tıklama Kontrolü
        uint32_t now = get_timer_ticks();
        if (fm_last_click_index == index && (now - fm_last_click_time) < 10) { // ~500ms
             // ÇİFT TIKLANDI! Dosyayı Aç.
             print("\nDosya Aciliyor: ");
             print(file_list[index].name);
             print(file_list[index].name);
             
             // Uzantı Kontrolü
             char* fname = file_list[index].name;
             int len = strlen(fname);
             if (len > 4) {
                 const char* ext = fname + len - 3;
                 if (strcmp(ext, "RAW") == 0 || strcmp(ext, "IMG") == 0 || strcmp(ext, "GUM") == 0) {
                     init_image_viewer(fname);
                 } else {
                     open_text_editor(fname);
                 }
             } else {
                 open_text_editor(fname);
             }
             
             // Resetle
             fm_last_click_index = -1;
        } else {
            // İlk Tık (Seçim)
            // İleride seçili dosyanın rengini değiştirebiliriz
            fm_last_click_index = index;
            fm_last_click_time = now;
        }
    }
}

void show_file_manager_window() {
    refresh_file_list();
    int id = create_window("Dosya Yonc.", 2, 3, 20, 15, (2 << 4) | 15); // Yeşil
    set_window_callbacks(id, file_manager_draw, file_manager_click);
    set_window_data(id, 0, 0); // Data yok, close handler yok
}

void init_file_manager() {
    show_file_manager_window();
}
