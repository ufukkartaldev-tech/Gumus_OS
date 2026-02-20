#ifndef WINDOW_H
#define WINDOW_H

#include <stdint.h>

#define MAX_WINDOWS 10

typedef struct window_t {
    int id;
    int x, y;
    int w, h;
    char title[32];
    uint8_t color;
    uint8_t z_index; // 0 en arkada, büyük olan önde
    uint8_t is_visible;
    
    // Callbackler
    void (*draw_content)(struct window_t* win);
    void (*on_click)(struct window_t* win, int x, int y);
    void (*on_key)(struct window_t* win, char c); // Klavye Girdisi
    void (*on_close)(struct window_t* win);
    void (*on_update)(struct window_t* win); // Her tick'te çağrılır
    
    // Pencereye özel veri (Instance Data)
    void* data;
} window_t;

// Pencere Yöneticisi Fonksiyonları
void init_window_manager();
int create_window(const char* title, int x, int y, int w, int h, uint8_t color);
void close_window(int id);
void focus_window(int id); // Pencereyi en öne getir
void draw_windows(); // Z-Order'a göre tüm pencereleri çiz
int get_window_at(int x, int y); // Belirtilen koordinattaki pencere ID'sini döner
void move_window(int id, int dx, int dy); // Pencereyi taşır
void on_window_click_event(int id, int x, int y); // Tıklama olayını pencereye iletir
int on_window_key_event(char c); // Klavye olayını odaklanmış pencereye iletir (1 dönerse işlendi)
typedef struct window_t window_t; 
void set_window_callbacks(int id, void (*draw)(window_t*), void (*click)(window_t*, int, int));
void set_window_key_callback(int id, void (*key)(window_t*, char));
void set_window_update_callback(int id, void (*update)(window_t*));
void set_window_data(int id, void* data, void (*on_close)(struct window_t*));
void update_windows(); // Tüm pencerelerin update fonksiyonunu çağırır
void draw_single_window(window_t* win, uint8_t is_active);

#endif
