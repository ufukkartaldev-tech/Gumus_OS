#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <stdint.h>
#include "../core/window.h"
#include "../drivers/vga_gfx.h"
#include "../drivers/advanced_sound.h"

// Ses Arayüzü Sabitleri
#define AUDIO_MIXER_WIDTH 70
#define AUDIO_MIXER_HEIGHT 25
#define CHANNEL_STRIP_WIDTH 8
#define CHANNEL_STRIP_HEIGHT 15
#define EQ_BANDS 10
#define VISUALIZER_BARS 32

// Ses Arayüzü Renkleri
#define AUDIO_BG_COLOR 15          // Beyaz
#define AUDIO_BORDER_COLOR 8       // Gri
#define AUDIO_TEXT_COLOR 0         // Siyah
#define AUDIO_ACTIVE_COLOR 4       // Kırmızı
#define AUDIO_METER_COLOR 2       // Yeşil
#define AUDIO_SLIDER_COLOR 1       // Mavi
#define AUDIO_PEAK_COLOR 12       // Açık kırmızı

// Ses Arayüzü Pencere Verisi
typedef struct {
    int selected_channel;
    int show_equalizer;
    int show_visualizer;
    int recording;
    
    // Equalizer verileri
    uint8_t eq_bands[EQ_BANDS];
    
    // Visualizer verileri
    uint8_t visualizer_bars[VISUALIZER_BARS];
    uint32_t visualizer_history[VISUALIZER_BARS][50];
    int visualizer_index;
    
    // Mouse durumu
    int mouse_x, mouse_y;
    int mouse_pressed;
    int dragged_channel;
    int dragged_eq_band;
} audio_mixer_ui_t;

// Ses Arayüzü Fonksiyonları
void audio_mixer_init();
void audio_mixer_draw_window(window_t* win);
void audio_mixer_handle_click(window_t* win, int x, int y);
void audio_mixer_handle_key(window_t* win, char c);
void audio_mixer_handle_mouse(window_t* win, int x, int y, int pressed);
void audio_mixer_update(window_t* win);

// Çizim fonksiyonları
void audio_draw_channel_strip(window_t* win, int channel, int x, int y, int width, int height);
void audio_draw_equalizer(window_t* win, int x, int y, int width, int height);
void audio_draw_visualizer(window_t* win, int x, int y, int width, int height);
void audio_draw_volume_slider(int x, int y, int width, int height, uint8_t volume, int active);
void audio_draw_pan_slider(int x, int y, int width, int height, uint8_t pan, int active);
void audio_draw_vu_meter(int x, int y, int width, int height, uint8_t level);

// Interaction fonksiyonları
int audio_get_channel_at_position(int x, int y);
int audio_get_eq_band_at_position(int x, int y);
void audio_handle_channel_click(int channel, int y);
void audio_handle_eq_click(int band, int y);
void audio_update_visualizer();

// Ana giriş fonksiyonu
void launch_audio_mixer();

#endif
