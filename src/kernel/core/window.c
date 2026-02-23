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

// Z-Index'i en yÃ¼ksek olanÄ± bul (Maksimum derinlik)
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
    // BoÅŸ slot bul
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
    
    // VarsayÄ±lan Callbackler
    windows[slot].draw_content = 0;
    windows[slot].on_click = 0;
    windows[slot].on_key = 0;
    windows[slot].on_close = 0;
    windows[slot].on_update = 0;
    windows[slot].data = 0;
    windows[slot].data = 0;
    
    // Yeni pencereyi en Ã¶ne koy
    windows[slot].z_index = get_top_z() + 1;
    
    // EkranÄ± gÃ¼ncelle
    draw_windows();
    
    return windows[slot].id;
}

void close_window(int id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            // Ã–nce temizlik callback'ini Ã§aÄŸÄ±r
            if (windows[i].on_close) {
                windows[i].on_close(&windows[i]);
            }
            
            windows[i].is_visible = 0;
            // Z-Index boÅŸluÄŸunu doldurmak gerekebilir ama ÅŸimdilik kalsÄ±n
            // draw_windows zaten visible kontrolÃ¼ yapÄ±yor
            
            // EkranÄ± temizleyip yeniden Ã§izmek gerekir Ã§Ã¼nkÃ¼ pencere kapandÄ±
            // vga_clear(0); // Bu Ã§ok kaba olur
            // Åimdilik sadece pencereleri yeniden Ã§izelim
            draw_windows();
            return;
        }
    }
}

window_t* get_window(int id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            return &windows[i];
        }
    }
    return 0;
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
    // Ã‡erÃ§eve rengi aktifse parlak, deÄŸilse soluk
    uint8_t border_color = is_active ? 15 : 8; 
    
    // draw_window fonksiyonu (kernel.c veya vga_gfx.c iÃ§inde olmalÄ±)
    // Åimdilik vga_gfx.c iÃ§inde draw_window olmadÄ±ÄŸÄ± iÃ§in kernel.c'dekini buraya taÅŸÄ±yoruz
    // veya vga_draw_rect ile manuel Ã§iziyoruz.
    
    // Pencere Ã§izim mantÄ±ÄŸÄ±nÄ± kernel.c'den buraya taÅŸÄ±dÄ±m (daha temiz)
    int px = win->x * 8;
    int py = win->y * 8;
    int pw = win->w * 8;
    int ph = win->h * 8;

    // GÃ¶lge Efekti (Shadow)
    // Pencerenin 4 piksel saÄŸÄ±na ve aÅŸaÄŸÄ±sÄ±na
    // Rengi: Koyu Gri (8) veya Siyah (0) ama zemin siyahsa gÃ¶rÃ¼nmez.
    // Arkaplan renkli olduÄŸu iÃ§in Siyah (0) veya Koyu Gri (8) iÅŸe yarar.
    // GÃ¶lgeyi sadece pencere aktifse veya her zaman Ã§izebiliriz.
    // Aktif pencerenin gÃ¶lgesi daha belirgin olabilir.
    
    int shadow_offset = 4;
    vga_draw_rect(px + shadow_offset, py + shadow_offset, pw, ph, 8); // Koyu Gri
    
    // Pencere GÃ¶vdesi (ÃœstÃ¼ne biner)
    vga_draw_rect(px, py, pw, ph, 0); // Ä°Ã§erik Siyah
    
    // Kenarlar
    vga_draw_rect(px, py, pw, 1, win->color); // Ãœst
    vga_draw_rect(px, py + ph - 1, pw, 1, win->color); // Alt
    vga_draw_rect(px, py, 1, ph, win->color); // Sol
    vga_draw_rect(px + pw - 1, py, 1, ph, win->color); // SaÄŸ

    // BaÅŸlÄ±k Ã‡ubuÄŸu ArkaplanÄ±
    // 10 piksel yÃ¼kseklik
    // Aktifse Mavi (1), Pasifse Gri (7)
    uint8_t title_bg = is_active ? 1 : 7; 
    uint8_t title_fg = is_active ? 15 : 8; // Beyaz veya Koyu Gri
    
    vga_draw_rect(px + 1, py + 1, pw - 2, 10, title_bg);

    // BaÅŸlÄ±k Metni
    int title_len = strlen(win->title);
    int title_px = px + (pw - (title_len * 8)) / 2;
    if (title_px < px) title_px = px;
    
    // Metni ortala ve dikey hizala (~2px offset)
    vga_draw_text(title_px, py + 2, win->title, title_fg);
    
    // Kapatma Butonu [X]
    // SaÄŸ Ã¼st kÃ¶ÅŸe
    int close_x = px + pw - 10;
    int close_y = py + 2;
    vga_draw_rect(close_x, close_y, 8, 8, 4); // KÄ±rmÄ±zÄ± Kutu
    vga_draw_char(close_x, close_y, 'X', 15); // Beyaz X
    
    // Ä°Ã§erik AlanÄ± Ã‡izgisi (BaÅŸlÄ±k ile iÃ§erik arasÄ±)
    vga_draw_rect(px, py + 11, pw, 1, win->color);
    
    // Ã–zel Ä°Ã§erik Ã‡izimi
    if (win->draw_content) {
        win->draw_content(win);
    }
}

void draw_windows() {
    // Z-Index'e gÃ¶re sÄ±ralÄ± Ã§izim (Bubble Sort ile sÄ±ralayalÄ±m)
    // KÃ¼Ã§Ã¼kten bÃ¼yÃ¼ÄŸe (Arkadan Ã¶ne)
    
    // GeÃ§ici dizi
    int sorted_indices[MAX_WINDOWS];
    int count = 0;
    
    for(int i=0; i<MAX_WINDOWS; i++) {
        if (windows[i].is_visible) {
            sorted_indices[count++] = i;
        }
    }
    
    // SÄ±rala
    for(int i=0; i<count-1; i++) {
        for(int j=0; j<count-i-1; j++) {
            if (windows[sorted_indices[j]].z_index > windows[sorted_indices[j+1]].z_index) {
                int temp = sorted_indices[j];
                sorted_indices[j] = sorted_indices[j+1];
                sorted_indices[j+1] = temp;
            }
        }
    }
    
    // Ã‡iz (En arkadakinden baÅŸlayarak)
    for(int i=0; i<count; i++) {
        int idx = sorted_indices[i];
        // En sondaki pencere aktiftir
        draw_single_window(&windows[idx], (i == count-1));
    }
}

// Koordinattaki pencereyi bul (Z-Index'i en yÃ¼ksek olan)
int get_window_at(int x, int y) {
    // x ve y piksel cinsinden, ama pencereler karakter (8x8) cinsinden saklanÄ±yor
    // KarÅŸÄ±laÅŸtÄ±rma yaparken piksele Ã§evireceÄŸiz
    
    int best_id = -1;
    int max_z = -1;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].is_visible) continue;
        
        int px = windows[i].x * 8;
        int py = windows[i].y * 8;
        int pw = windows[i].w * 8;
        int ph = windows[i].h * 8;
        
        // BaÅŸlÄ±k Ã§ubuÄŸu dahil (py'den itibaren)
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
    // Ancak pencereler karakter gridinde (8 piksel) hareket etmeli (basitleÅŸtirme)
    // Bu yÃ¼zden hassas hareket iÃ§in float/subpixel gerekir ama biz kaba hareket yapacaÄŸÄ±z.
    // Her 8 pikselde 1 birim karakter hareketi.
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            
            // Piksel hareketini biriktirmek iÃ§in statik/global deÄŸiÅŸken lazÄ±m.
            // Åimdilik kaba yÃ¶ntem: dx 4'ten bÃ¼yÃ¼kse hareket et
            
            // Grid tabanlÄ± hareket (Daha stabil)
            // windows[i].x += dx / 8; 
            // windows[i].y += dy / 8;
            
            // Ama mouse Ã§ok hÄ±zlÄ± hareket ederse ve dx < 8 ise pencere hareket etmez.
            // Bu yÃ¼zden "pencereyi sÃ¼rÃ¼kle" modunda mutlak mouse pozisyonuna gÃ¶re 
            // yeniden hesaplama yapmak daha doÄŸru ama ÅŸimdilik "yavaÅŸ" sÃ¼rÃ¼kleme yeterli.
            
            // EÄŸer dx pozitifse 1 arttÄ±r, negatifse 1 azalt (Her tick'te max 1 karakter)
            // Bu Ã§ok hÄ±zlÄ± olur. O yÃ¼zden sadece belli bir eÅŸiÄŸi geÃ§ince.
            
            // Basit Hack: Pencere koordinatlarÄ±nÄ± piksel hassasiyetinde tutmak lazÄ±m.
            // Ama struct int x,y character grid.
            // Åimdilik sadece yÃ¶n varsa hareket ettir (Ã‡ok hÄ±zlÄ± olacak ama test edelim)
            
            if (dx > 0) windows[i].x++;
            if (dx < 0) windows[i].x--;
            if (dy > 0) windows[i].y++; // Y ekseni aÅŸaÄŸÄ± artar
            if (dy < 0) windows[i].y--;
            if (dy > 0) windows[i].y++; // Y ekseni aÅŸaÄŸÄ± artar
            if (dy < 0) windows[i].y--;
            // draw_windows(); // Timer Loop Ã§izecek
            return;
            return;
        }
    }
}

void on_window_click_event(int id, int x, int y) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i].is_visible && windows[i].id == id) {
            // Pencere iÃ§i koordinatlara Ã§evir (BaÅŸlÄ±k Ã§ubuÄŸu hariÃ§ deÄŸil, tamamÄ±)
            // draw_single_window referansÄ±: px, py baÅŸlangÄ±Ã§.
            // Mouse x, y mutlak.
            int local_x = x - (windows[i].x * 8);
            int local_y = y - (windows[i].y * 8);
            
            // Kapatma Butonu KontrolÃ¼
            // SaÄŸ Ã¼stteki 10-12 piksellik alan
            int win_width_px = windows[i].w * 8;
            if (local_y < 12 && local_x > win_width_px - 12) {
                // Kapatma Butonuna TÄ±klandÄ±!
                close_window(windows[i].id);
                return;
            }
            
            // BaÅŸlÄ±k Ã§ubuÄŸuna tÄ±klanÄ±rsa (ilk 8-12 piksel) taÅŸÄ±madÄ±r, iÃ§erik deÄŸil.
            // Ama callback'e biz ham veriyi atalÄ±m, o karar versin.
            
            if (windows[i].on_click) {
                windows[i].on_click(&windows[i], local_x, local_y);
            }
            return;
        }
    }
}

int on_window_key_event(char c) {
    // OdaklanmÄ±ÅŸ (En Ã¼stteki) pencereyi bul
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
    
    // EÄŸer bir pencere varsa ve key handler'Ä± varsa
    if (idx != -1 && windows[idx].on_key) {
        windows[idx].on_key(&windows[idx], c);
        return 1; // TuÅŸ iÅŸlendi
    }
    
    return 0; // TuÅŸ iÅŸlenmedi (Shell'e gidebilir)
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
