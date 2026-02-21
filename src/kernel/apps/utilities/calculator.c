#include "calculator.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "string.h"
#include "memory.h"

// Hesap Makinesi Durumu
typedef struct {
    char display[16];   // Ekranda görünen sayı
    int current_val;    // Girilen sayı
    int stored_val;     // İşlem için saklanan sayı
    char operation;     // +, -, *, /
    int new_entry;      // Yeni sayı girişi mi bekleniyor?
} calculator_t;

// Buton Düzeni
// 7 8 9 +
// 4 5 6 -
// 1 2 3 *
// C 0 = /

void calculator_draw(window_t* win) {
    calculator_t* calc = (calculator_t*)win->data;
    if (!calc) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    
    // Ekran Alanı (Beyaz Arkaplan)
    vga_draw_rect(px + 10, py + 20, 140, 20, 15);
    
    // Sayıyı Sağa Dayalı Yaz
    int len = strlen(calc->display);
    vga_draw_text(px + 140 - (len * 8), py + 26, calc->display, 0);
    
    // Buton Çizimi
    const char* buttons[] = {
        "7", "8", "9", "+",
        "4", "5", "6", "-",
        "1", "2", "3", "*",
        "C", "0", "=", "/"
    };
    
    for (int i = 0; i < 16; i++) {
        int r = i / 4;
        int c = i % 4;
        
        int btn_x = px + 10 + (c * 35);
        int btn_y = py + 50 + (r * 25);
        
        // Buton Kutusu (Gri)
        vga_draw_rect(btn_x, btn_y, 30, 20, 7);
        // Gölge (3D Efekt)
        vga_draw_line(btn_x, btn_y + 20, btn_x + 30, btn_y + 20, 0); // Alt Siyah
        vga_draw_line(btn_x + 30, btn_y, btn_x + 30, btn_y + 20, 0); // Sağ Siyah
        vga_draw_line(btn_x, btn_y, btn_x + 30, btn_y, 15); // Üst Beyaz
        vga_draw_line(btn_x, btn_y, btn_x, btn_y + 20, 15); // Sol Beyaz
        
        // Buton Yazısı
        vga_draw_text(btn_x + 11, btn_y + 6, buttons[i], 0);
    }
}

void process_button(calculator_t* calc, char c) {
    if (c >= '0' && c <= '9') {
        if (calc->new_entry) {
            calc->display[0] = c;
            calc->display[1] = '\0';
            calc->new_entry = 0;
        } else {
            int len = strlen(calc->display);
            if (len < 10) { // Max 10 hane
                calc->display[len] = c;
                calc->display[len+1] = '\0';
            }
        }
        calc->current_val = atoi(calc->display);
    } else if (c == 'C') {
        calc->current_val = 0;
        calc->stored_val = 0;
        calc->operation = 0;
        strcpy(calc->display, "0");
        calc->new_entry = 1;
    } else if (c == '+' || c == '-' || c == '*' || c == '/') {
        calc->stored_val = calc->current_val;
        calc->operation = c;
        calc->new_entry = 1;
    } else if (c == '=') {
        int result = 0;
        if (calc->operation == '+') result = calc->stored_val + calc->current_val;
        else if (calc->operation == '-') result = calc->stored_val - calc->current_val;
        else if (calc->operation == '*') result = calc->stored_val * calc->current_val;
        else if (calc->operation == '/') {
            if (calc->current_val != 0) result = calc->stored_val / calc->current_val;
            else result = 0; // Hata yönetimi yok
        } else {
            result = calc->current_val;
        }
        
        itoa(result, calc->display);
        calc->current_val = result;
        calc->operation = 0; // İşlemi sıfırla
        calc->new_entry = 1;
    }
}

void calculator_click(window_t* win, int x, int y) {
    calculator_t* calc = (calculator_t*)win->data;
    if (!calc) return;
    
    // Pencere içi koordinatlar (x, y)
    // Buton alanı başlıyor: y=50
    if (y < 50) return;
    
    // Buton tespiti
    int r = (y - 50) / 25;
    int c = (x - 10) / 35;
    
    if (r >= 0 && r < 4 && c >= 0 && c < 4) {
        // Tıklanan buton indeksi
        int idx = r * 4 + c;
        const char* buttons[] = {
            "7", "8", "9", "+",
            "4", "5", "6", "-",
            "1", "2", "3", "*",
            "C", "0", "=", "/"
        };
        
        char btn_char = buttons[idx][0];
        process_button(calc, btn_char);
        
        // Ekranı güncelle
        // draw_single_window(win, 1); // Render loop halledecek
    }
}

// Klavye desteği (Opsiyonel ama şık olur)
void calculator_key(window_t* win, char c) {
    calculator_t* calc = (calculator_t*)win->data;
    if (!calc) return;
    
    // Enter -> =
    if (c == 13) c = '=';
    
    // Geçerli tuşları filtrele
    if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == 'C' || c == 'c') {
        if (c == 'c') c = 'C';
        process_button(calc, c);
        // draw_single_window(win, 1);
    }
}

void calculator_close(window_t* win) {
    if (win->data) {
        kfree(win->data);
    }
}

void init_calculator() {
    calculator_t* calc = (calculator_t*)kmalloc(sizeof(calculator_t));
    calc->current_val = 0;
    calc->stored_val = 0;
    calc->operation = 0;
    strcpy(calc->display, "0");
    calc->new_entry = 1;
    
    // 160x160 boyutunda pencere
    int win_id = create_window("Hesap Makinesi", 10, 5, 20, 20, (7 << 4) | 0); // Gri, Siyah kenar
    
    set_window_callbacks(win_id, calculator_draw, calculator_click);
    set_window_key_callback(win_id, calculator_key);
    set_window_data(win_id, calc, calculator_close);
}
