#include "window.h"
#include "vga_gfx.h"
#include "string.h"

static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int next_window_id = 1;

void init_window_manager() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_visible = 0;
        windows[i].z_index = 0;
    }
}

// Z-Index'i en yüksek olanı bul (Maksimum derinlik)
int get_top_z() {
    int max_z = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].z_index > max_z) {
            max_z = windows[i].z_index;
        }
    }
    return max_z;
}

int create_window(const char* title, int x, int y, int w, int h, uint8_t color) {
    // Boş slot bul
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].is_visible) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return -1; // Yer yok

    windows[slot].id = next_window_id++;
    windows[slot].x = x;
    windows[slot].y = y;
    windows[slot].w = w;
    windows[slot].h = h;
    strcpy(windows[slot].title, title);
    windows[slot].color = color;
    windows[slot].is_visible = 1;
    
    // Varsayılan Callbackler
    windows[slot].draw_content = 0;
    windows[slot].on_click = 0;
    windows[slot].on_key = 0;
    windows[slot].on_close = 0;
    windows[slot].on_update = 0;
    windows[slot].data = 0;
    windows[slot].data = 0;
    
    // Yeni pencereyi en öne koy
    windows[slot].z_index = get_top_z() + 1;
    
    // Ekranı güncelle
    draw_windows();
    
    return windows[slot].id;
}

void close_window(int id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            // Önce temizlik callback'ini çağır
            if (windows[i].on_close) {
                windows[i].on_close(&windows[i]);
            }
            
            windows[i].is_visible = 0;
            // Z-Index boşluğunu doldurmak gerekebilir ama şimdilik kalsın
            // draw_windows zaten visible kontrolü yapıyor
            
            // Ekranı temizleyip yeniden çizmek gerekir çünkü pencere kapandı
            // vga_clear(0); // Bu çok kaba olur
            // Şimdilik sadece pencereleri yeniden çizelim
            draw_windows();
            return;
        }
    }
}

void focus_window(int id) {
    int target_idx = -1;
    for(int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            target_idx = i;
            break;
        }
    }
    
    if (target_idx != -1) {
        windows[target_idx].z_index = get_top_z() + 1;
        draw_windows();
    }
}

void draw_single_window(window_t* win, uint8_t is_active) {
    // Çerçeve rengi aktifse parlak, değilse soluk
    uint8_t border_color = is_active ? 15 : 8; 
    
    // draw_window fonksiyonu (kernel.c veya vga_gfx.c içinde olmalı)
    // Şimdilik vga_gfx.c içinde draw_window olmadığı için kernel.c'dekini buraya taşıyoruz
    // veya vga_draw_rect ile manuel çiziyoruz.
    
    // Pencere çizim mantığını kernel.c'den buraya taşıdım (daha temiz)
    int px = win->x * 8;
    int py = win->y * 8;
    int pw = win->w * 8;
    int ph = win->h * 8;

    // Gölge Efekti (Shadow)
    // Pencerenin 4 piksel sağına ve aşağısına
    // Rengi: Koyu Gri (8) veya Siyah (0) ama zemin siyahsa görünmez.
    // Arkaplan renkli olduğu için Siyah (0) veya Koyu Gri (8) işe yarar.
    // Gölgeyi sadece pencere aktifse veya her zaman çizebiliriz.
    // Aktif pencerenin gölgesi daha belirgin olabilir.
    
    int shadow_offset = 4;
    vga_draw_rect(px + shadow_offset, py + shadow_offset, pw, ph, 8); // Koyu Gri
    
    // Pencere Gövdesi (Üstüne biner)
    vga_draw_rect(px, py, pw, ph, 0); // İçerik Siyah
    
    // Kenarlar
    vga_draw_rect(px, py, pw, 1, win->color); // Üst
    vga_draw_rect(px, py + ph - 1, pw, 1, win->color); // Alt
    vga_draw_rect(px, py, 1, ph, win->color); // Sol
    vga_draw_rect(px + pw - 1, py, 1, ph, win->color); // Sağ

    // Başlık Çubuğu Arkaplanı
    // 10 piksel yükseklik
    // Aktifse Mavi (1), Pasifse Gri (7)
    uint8_t title_bg = is_active ? 1 : 7; 
    uint8_t title_fg = is_active ? 15 : 8; // Beyaz veya Koyu Gri
    
    vga_draw_rect(px + 1, py + 1, pw - 2, 10, title_bg);

    // Başlık Metni
    int title_len = strlen(win->title);
    int title_px = px + (pw - (title_len * 8)) / 2;
    if (title_px < px) title_px = px;
    
    // Metni ortala ve dikey hizala (~2px offset)
    vga_draw_text(title_px, py + 2, win->title, title_fg);
    
    // Kapatma Butonu [X]
    // Sağ üst köşe
    int close_x = px + pw - 10;
    int close_y = py + 2;
    vga_draw_rect(close_x, close_y, 8, 8, 4); // Kırmızı Kutu
    vga_draw_char(close_x, close_y, 'X', 15); // Beyaz X
    
    // İçerik Alanı Çizgisi (Başlık ile içerik arası)
    vga_draw_rect(px, py + 11, pw, 1, win->color);
    
    // Özel İçerik Çizimi
    if (win->draw_content) {
        win->draw_content(win);
    }
}

void draw_windows() {
    // Z-Index'e göre sıralı çizim (Bubble Sort ile sıralayalım)
    // Küçükten büyüğe (Arkadan öne)
    
    // Geçici dizi
    int sorted_indices[MAX_WINDOWS];
    int count = 0;
    
    for(int i=0; i<MAX_WINDOWS; i++) {
        if (windows[i].is_visible) {
            sorted_indices[count++] = i;
        }
    }
    
    // Sırala
    for(int i=0; i<count-1; i++) {
        for(int j=0; j<count-i-1; j++) {
            if (windows[sorted_indices[j]].z_index > windows[sorted_indices[j+1]].z_index) {
                int temp = sorted_indices[j];
                sorted_indices[j] = sorted_indices[j+1];
                sorted_indices[j+1] = temp;
            }
        }
    }
    
    // Çiz (En arkadakinden başlayarak)
    for(int i=0; i<count; i++) {
        int idx = sorted_indices[i];
        // En sondaki pencere aktiftir
        draw_single_window(&windows[idx], (i == count-1));
    }
}

// Koordinattaki pencereyi bul (Z-Index'i en yüksek olan)
int get_window_at(int x, int y) {
    // x ve y piksel cinsinden, ama pencereler karakter (8x8) cinsinden saklanıyor
    // Karşılaştırma yaparken piksele çevireceğiz
    
    int best_id = -1;
    int max_z = -1;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].is_visible) continue;
        
        int px = windows[i].x * 8;
        int py = windows[i].y * 8;
        int pw = windows[i].w * 8;
        int ph = windows[i].h * 8;
        
        // Başlık çubuğu dahil (py'den itibaren)
        if (x >= px && x < px + pw && y >= py && y < py + ph) {
            if (windows[i].z_index > max_z) {
                max_z = windows[i].z_index;
                best_id = windows[i].id;
            }
        }
    }
    return best_id;
}

void move_window(int id, int dx, int dy) {
    // dx, dy piksel hareketi
    // Ancak pencereler karakter gridinde (8 piksel) hareket etmeli (basitleştirme)
    // Bu yüzden hassas hareket için float/subpixel gerekir ama biz kaba hareket yapacağız.
    // Her 8 pikselde 1 birim karakter hareketi.
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            
            // Piksel hareketini biriktirmek için statik/global değişken lazım.
            // Şimdilik kaba yöntem: dx 4'ten büyükse hareket et
            
            // Grid tabanlı hareket (Daha stabil)
            // windows[i].x += dx / 8; 
            // windows[i].y += dy / 8;
            
            // Ama mouse çok hızlı hareket ederse ve dx < 8 ise pencere hareket etmez.
            // Bu yüzden "pencereyi sürükle" modunda mutlak mouse pozisyonuna göre 
            // yeniden hesaplama yapmak daha doğru ama şimdilik "yavaş" sürükleme yeterli.
            
            // Eğer dx pozitifse 1 arttır, negatifse 1 azalt (Her tick'te max 1 karakter)
            // Bu çok hızlı olur. O yüzden sadece belli bir eşiği geçince.
            
            // Basit Hack: Pencere koordinatlarını piksel hassasiyetinde tutmak lazım.
            // Ama struct int x,y character grid.
            // Şimdilik sadece yön varsa hareket ettir (Çok hızlı olacak ama test edelim)
            
            if (dx > 0) windows[i].x++;
            if (dx < 0) windows[i].x--;
            if (dy > 0) windows[i].y++; // Y ekseni aşağı artar
            if (dy < 0) windows[i].y--;
            if (dy > 0) windows[i].y++; // Y ekseni aşağı artar
            if (dy < 0) windows[i].y--;
            // draw_windows(); // Timer Loop çizecek
            return;
            return;
        }
    }
}

void on_window_click_event(int id, int x, int y) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            // Pencere içi koordinatlara çevir (Başlık çubuğu hariç değil, tamamı)
            // draw_single_window referansı: px, py başlangıç.
            // Mouse x, y mutlak.
            int local_x = x - (windows[i].x * 8);
            int local_y = y - (windows[i].y * 8);
            
            // Kapatma Butonu Kontrolü
            // Sağ üstteki 10-12 piksellik alan
            int win_width_px = windows[i].w * 8;
            if (local_y < 12 && local_x > win_width_px - 12) {
                // Kapatma Butonuna Tıklandı!
                close_window(windows[i].id);
                return;
            }
            
            // Başlık çubuğuna tıklanırsa (ilk 8-12 piksel) taşımadır, içerik değil.
            // Ama callback'e biz ham veriyi atalım, o karar versin.
            
            if (windows[i].on_click) {
                windows[i].on_click(&windows[i], local_x, local_y);
            }
            return;
        }
    }
}

int on_window_key_event(char c) {
    // Odaklanmış (En üstteki) pencereyi bul
    int top_id = -1;
    int max_z = -1;
    int idx = -1;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible) {
            if (windows[i].z_index > max_z) {
                max_z = windows[i].z_index;
                top_id = windows[i].id;
                idx = i;
            }
        }
    }
    
    // Eğer bir pencere varsa ve key handler'ı varsa
    if (idx != -1 && windows[idx].on_key) {
        windows[idx].on_key(&windows[idx], c);
        return 1; // Tuş işlendi
    }
    
    return 0; // Tuş işlenmedi (Shell'e gidebilir)
}

void set_window_callbacks(int id, void (*draw)(window_t*), void (*click)(window_t*, int, int)) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            windows[i].draw_content = draw;
            windows[i].on_click = click;
            return;
        }
    }
}

void set_window_key_callback(int id, void (*key)(window_t*, char)) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            windows[i].on_key = key;
            return;
        }
    }
}

void set_window_data(int id, void* data, void (*on_close)(struct window_t*)) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            windows[i].data = data;
            windows[i].on_close = on_close;
            return;
        }
    }
}

void set_window_update_callback(int id, void (*update)(window_t*)) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            windows[i].on_update = update;
            return;
        }
    }
}

void update_windows() {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].on_update) {
            windows[i].on_update(&windows[i]);
        }
    }
}
