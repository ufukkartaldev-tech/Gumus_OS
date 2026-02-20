#include "audio_mixer.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "string.h"
#include "memory.h"
#include "../drivers/advanced_sound.h"
#include "../drivers/mouse.h"

// Global değişkenler
static audio_mixer_ui_t* mixer_ui = NULL;
static int audio_mixer_initialized = 0;

void audio_mixer_init() {
    if (audio_mixer_initialized) return;
    
    mixer_ui = (audio_mixer_ui_t*)kmalloc(sizeof(audio_mixer_ui_t));
    if (!mixer_ui) return;
    
    memset(mixer_ui, 0, sizeof(audio_mixer_ui_t));
    mixer_ui->selected_channel = 0;
    mixer_ui->show_equalizer = 1;
    mixer_ui->show_visualizer = 1;
    mixer_ui->recording = 0;
    
    // Varsayılan EQ ayarları
    for (int i = 0; i < EQ_BANDS; i++) {
        mixer_ui->eq_bands[i] = 50; // Orta seviye
    }
    
    // Visualizer'ı temizle
    memset(mixer_ui->visualizer_history, 0, sizeof(mixer_ui->visualizer_history));
    mixer_ui->visualizer_index = 0;
    
    audio_mixer_initialized = 1;
    printf("Ses Mixer Arayüzü başlatıldı\n");
}

void audio_draw_volume_slider(int x, int y, int width, int height, uint8_t volume, int active) {
    // Arka plan
    vga_draw_rect(x, y, width, height, AUDIO_BG_COLOR);
    
    // Çerçeve
    if (active) {
        vga_draw_rect(x, y, width, height, AUDIO_ACTIVE_COLOR);
    } else {
        vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
    }
    
    // Slider dolgusu
    int fill_height = (volume * (height - 4)) / 100;
    int slider_y = y + height - 2 - fill_height;
    
    vga_draw_rect(x + 2, slider_y, width - 4, fill_height, AUDIO_SLIDER_COLOR);
    
    // Yüzde text'i
    char vol_text[8];
    sprintf(vol_text, "%d%%", volume);
    int text_x = x + (width - strlen(vol_text) * 8) / 2;
    vga_draw_text(text_x, y + height + 2, vol_text, AUDIO_TEXT_COLOR);
}

void audio_draw_pan_slider(int x, int y, int width, int height, uint8_t pan, int active) {
    // Arka plan
    vga_draw_rect(x, y, width, height, AUDIO_BG_COLOR);
    
    // Çerçeve
    if (active) {
        vga_draw_rect(x, y, width, height, AUDIO_ACTIVE_COLOR);
    } else {
        vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
    }
    
    // Pan slider'ı (merkezden sola/sağa)
    int slider_x = x + 2 + (pan * (width - 4)) / 255;
    
    vga_draw_rect(slider_x - 1, y + 2, 2, height - 4, AUDIO_SLIDER_COLOR);
    
    // Merkez çizgisi
    int center_x = x + width / 2;
    vga_draw_line(center_x, y, center_x, y + height, AUDIO_BORDER_COLOR);
    
    // Text'ler
    vga_draw_text(x, y + height + 2, "L", AUDIO_TEXT_COLOR);
    vga_draw_text(x + width - 8, y + height + 2, "R", AUDIO_TEXT_COLOR);
}

void audio_draw_vu_meter(int x, int y, int width, int height, uint8_t level) {
    // Arka plan
    vga_draw_rect(x, y, width, height, AUDIO_BG_COLOR);
    
    // Çerçeve
    vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
    
    // VU meter dolgusu
    int fill_width = (level * (width - 2)) / 100;
    
    // Renk seviyesine göre (yeşil -> sarı -> kırmızı)
    uint8_t color = AUDIO_METER_COLOR;
    if (level > 80) color = AUDIO_PEAK_COLOR;
    else if (level > 60) color = 14; // Sarı
    
    vga_draw_rect(x + 1, y + 1, fill_width, height - 2, color);
}

void audio_draw_channel_strip(window_t* win, int channel, int x, int y, int width, int height) {
    if (!mixer_ui) return;
    
    // Kanal başlığı
    char title[16];
    sprintf(title, "CH %d", channel + 1);
    
    if (mixer_ui->selected_channel == channel) {
        vga_draw_rect(x, y, width, height, AUDIO_ACTIVE_COLOR);
        vga_draw_text(x + 2, y + 2, title, AUDIO_BG_COLOR);
    } else {
        vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
        vga_draw_text(x + 2, y + 2, title, AUDIO_TEXT_COLOR);
    }
    
    int content_y = y + 15;
    
    // Volume slider
    audio_draw_volume_slider(x + 2, content_y, width - 4, 8, 
                           audio_mixer.channels[channel].volume, 
                           mixer_ui->selected_channel == channel);
    content_y += 20;
    
    // Pan slider
    audio_draw_pan_slider(x + 2, content_y, width - 4, 6, 
                         audio_mixer.channels[channel].pan,
                         mixer_ui->selected_channel == channel);
    content_y += 15;
    
    // VU meter
    uint8_t vu_level = audio_is_playing(channel) ? 70 : 0; // Simüle edilmiş seviye
    audio_draw_vu_meter(x + 2, content_y, width - 4, 4, vu_level);
    
    // Mute/Loop butonları
    int button_y = content_y + 8;
    vga_draw_text(x + 2, button_y, audio_mixer.channels[channel].loop ? "LOOP" : "----", 
                  audio_mixer.channels[channel].loop ? AUDIO_SLIDER_COLOR : AUDIO_TEXT_COLOR);
}

void audio_draw_equalizer(window_t* win, int x, int y, int width, int height) {
    if (!mixer_ui) return;
    
    // Başlık
    vga_draw_text(x, y, "Equalizer", AUDIO_TEXT_COLOR);
    y += 15;
    
    // EQ arka plan
    vga_draw_rect(x, y, width, height, AUDIO_BG_COLOR);
    vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
    
    // EQ band'ları
    int band_width = (width - 4) / EQ_BANDS;
    
    for (int i = 0; i < EQ_BANDS; i++) {
        int band_x = x + 2 + i * band_width;
        int band_height = (mixer_ui->eq_bands[i] * (height - 4)) / 100;
        int band_y = y + height - 2 - band_height;
        
        vga_draw_rect(band_x, band_y, band_width - 2, band_height, AUDIO_SLIDER_COLOR);
        
        // Frekans etiketleri
        if (i % 2 == 0) {
            char freq[8];
            sprintf(freq, "%d", 31 * (i + 1)); // Basit frekans hesabı
            vga_draw_text(band_x, y + height + 2, freq, AUDIO_TEXT_COLOR);
        }
    }
}

void audio_draw_visualizer(window_t* win, int x, int y, int width, int height) {
    if (!mixer_ui) return;
    
    // Başlık
    vga_draw_text(x, y, "Visualizer", AUDIO_TEXT_COLOR);
    y += 15;
    
    // Visualizer arka plan
    vga_draw_rect(x, y, width, height, AUDIO_BG_COLOR);
    vga_draw_rect(x, y, width, height, AUDIO_BORDER_COLOR);
    
    // Visualizer bar'ları
    int bar_width = (width - 2) / VISUALIZER_BARS;
    
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        int bar_x = x + 1 + i * bar_width;
        int bar_height = mixer_ui->visualizer_bars[i];
        int bar_y = y + height - 1 - bar_height;
        
        // Renk seviyesine göre
        uint8_t color = AUDIO_METER_COLOR;
        if (bar_height > height * 0.8) color = AUDIO_PEAK_COLOR;
        else if (bar_height > height * 0.6) color = 14; // Sarı
        
        vga_draw_rect(bar_x, bar_y, bar_width - 1, bar_height, color);
    }
    
    // Kayıt göstergesi
    if (mixer_ui->recording) {
        vga_draw_text(x + width - 60, y, "● REC", AUDIO_PEAK_COLOR);
    }
}

void audio_mixer_draw_window(window_t* win) {
    if (!audio_mixer_initialized || !mixer_ui) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    int height = win->h * 8;
    
    // Ana arka plan
    vga_draw_rect(px + 1, py + 1, width - 2, height - 2, AUDIO_BG_COLOR);
    
    // Bölüm ayırıcı çizgiler
    int channel_area_width = (AUDIO_MAX_CHANNELS * CHANNEL_STRIP_WIDTH * 8) + 20;
    
    if (channel_area_width < width - 200) {
        vga_draw_line(px + channel_area_width, py + 20, 
                     px + channel_area_width, py + height - 2, AUDIO_BORDER_COLOR);
    }
    
    // Kanal strip'leri
    int channel_x = px + 10;
    int channel_y = py + 25;
    
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (channel_x + CHANNEL_STRIP_WIDTH * 8 > width - 150) break;
        
        audio_draw_channel_strip(win, i, channel_x, channel_y, 
                              CHANNEL_STRIP_WIDTH * 8, CHANNEL_STRIP_HEIGHT * 8);
        channel_x += CHANNEL_STRIP_WIDTH * 8 + 5;
    }
    
    // Equalizer
    if (mixer_ui->show_equalizer && width > 400) {
        int eq_x = px + width - 180;
        int eq_y = py + 25;
        int eq_width = 160;
        int eq_height = 80;
        
        audio_draw_equalizer(win, eq_x, eq_y, eq_width, eq_height);
    }
    
    // Visualizer
    if (mixer_ui->show_visualizer && width > 400) {
        int vis_x = px + width - 180;
        int vis_y = py + 120;
        int vis_width = 160;
        int vis_height = 60;
        
        audio_draw_visualizer(win, vis_x, vis_y, vis_width, vis_height);
    }
    
    // Master volume
    int master_x = px + width - 180;
    int master_y = py + height - 50;
    
    vga_draw_text(master_x, master_y, "Master", AUDIO_TEXT_COLOR);
    audio_draw_volume_slider(master_x, master_y + 15, 150, 8, 
                           audio_mixer.master_volume, 1);
    
    // Durum bilgileri
    char status[64];
    sprintf(status, "CPU: %.1f%% | Channels: %d/%d", 
            audio_get_cpu_usage(), 
            audio_get_active_channels(), AUDIO_MAX_CHANNELS);
    vga_draw_text(px + 10, py + height - 15, status, AUDIO_TEXT_COLOR);
}

int audio_get_channel_at_position(int x, int y) {
    // Kanal pozisyonunu hesapla
    int channel_x = 10; // Başlangıç X pozisyonu
    
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        int channel_width = CHANNEL_STRIP_WIDTH * 8;
        
        if (x >= channel_x && x < channel_x + channel_width &&
            y >= 25 && y < 25 + CHANNEL_STRIP_HEIGHT * 8) {
            return i;
        }
        
        channel_x += channel_width + 5;
    }
    
    return -1;
}

int audio_get_eq_band_at_position(int x, int y) {
    if (!mixer_ui->show_equalizer) return -1;
    
    // EQ pozisyonunu hesapla (sağ taraf)
    int eq_x = 70 * 8 - 180; // Sağ taraf X pozisyonu
    int eq_y = 25; // EQ başlangıç Y pozisyonu
    
    if (x >= eq_x && x < eq_x + 160 &&
        y >= eq_y + 15 && y < eq_y + 95) {
        
        int band_width = 160 / EQ_BANDS;
        int band = (x - eq_x - 2) / band_width;
        
        if (band >= 0 && band < EQ_BANDS) {
            return band;
        }
    }
    
    return -1;
}

void audio_handle_channel_click(int channel, int y) {
    if (!mixer_ui || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    mixer_ui->selected_channel = channel;
    
    // Volume ayarlama
    if (y >= 40 && y < 48) {
        int new_volume = (y - 40) * 100 / 8;
        if (new_volume > 100) new_volume = 100;
        if (new_volume < 0) new_volume = 0;
        
        audio_set_volume(channel, new_volume);
    }
    
    // Pan ayarlama
    if (y >= 60 && y < 66) {
        int new_pan = (y - 60) * 255 / 6;
        if (new_pan > 255) new_pan = 255;
        if (new_pan < 0) new_pan = 0;
        
        audio_set_pan(channel, new_pan);
    }
}

void audio_handle_eq_click(int band, int y) {
    if (!mixer_ui || band < 0 || band >= EQ_BANDS) return;
    
    // EQ band ayarlama
    int eq_y = 25 + 15;
    int eq_height = 80;
    
    if (y >= eq_y && y < eq_y + eq_height) {
        int new_level = 100 - ((y - eq_y) * 100 / (eq_height - 4));
        if (new_level > 100) new_level = 100;
        if (new_level < 0) new_level = 0;
        
        mixer_ui->eq_bands[band] = new_level;
        
        // Efekt olarak uygula (basitleştirilmiş)
        audio_effect_params_t eq_effect = {
            .type = EFFECT_ECHO,
            .intensity = new_level / 100.0,
            .delay = 100,
            .feedback = 50
        };
        
        audio_clear_effects();
        audio_add_effect(&eq_effect);
    }
}

void audio_update_visualizer() {
    if (!mixer_ui) return;
    
    // Basit visualizer simülasyonu
    for (int i = 0; i < VISUALIZER_BARS; i++) {
        // Rastgele dalga formu oluştur
        static uint32_t wave_phase = 0;
        wave_phase += 10;
        
        int base_value = 20;
        int amplitude = 15;
        int frequency = 2 + i;
        
        mixer_ui->visualizer_bars[i] = base_value + 
            amplitude * sin((wave_phase + i * 30) * frequency * 3.14159 / 180.0);
        
        if (mixer_ui->visualizer_bars[i] < 0) 
            mixer_ui->visualizer_bars[i] = 0;
        if (mixer_ui->visualizer_bars[i] > 40) 
            mixer_ui->visualizer_bars[i] = 40;
    }
}

void audio_mixer_handle_click(window_t* win, int x, int y) {
    if (!audio_mixer_initialized || !mixer_ui) return;
    
    // Pencere koordinatlarını dönüştür
    int px = x - win->x * 8;
    int py = y - win->y * 8;
    
    // Kanal kontrolü
    int channel = audio_get_channel_at_position(px, py);
    if (channel >= 0) {
        audio_handle_channel_click(channel, py);
        return;
    }
    
    // EQ kontrolü
    int eq_band = audio_get_eq_band_at_position(px, py);
    if (eq_band >= 0) {
        audio_handle_eq_click(eq_band, py);
        return;
    }
    
    // Master volume kontrolü
    int master_x = 70 * 8 - 180;
    int master_y = 25 * 8 - 50;
    
    if (px >= master_x && px < master_x + 150 &&
        py >= master_y + 15 && py < master_y + 23) {
        
        int new_volume = 100 - ((py - master_y - 15) * 100 / 8);
        if (new_volume > 100) new_volume = 100;
        if (new_volume < 0) new_volume = 0;
        
        audio_set_master_volume(new_volume);
    }
}

void audio_mixer_handle_key(window_t* win, char c) {
    if (!audio_mixer_initialized || !mixer_ui) return;
    
    switch (c) {
        case 'r':
        case 'R':
            // Kayıt başlat/durdur
            if (mixer_ui->recording) {
                audio_stop_recording();
                mixer_ui->recording = 0;
            } else {
                audio_start_recording();
                mixer_ui->recording = 1;
            }
            break;
            
        case 'e':
        case 'E':
            // Equalizer göster/gizle
            mixer_ui->show_equalizer = !mixer_ui->show_equalizer;
            break;
            
        case 'v':
        case 'V':
            // Visualizer göster/gizle
            mixer_ui->show_visualizer = !mixer_ui->show_visualizer;
            break;
            
        case 's':
        case 'S':
            // Tüm sesi durdur
            for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
                audio_stop_channel(i);
            }
            break;
            
        case 'q':
        case 'Q':
            // Pencereyi kapat
            close_window(win->id);
            break;
    }
}

void audio_mixer_update(window_t* win) {
    if (!audio_mixer_initialized || !mixer_ui) return;
    
    // Visualizer'ı güncelle
    audio_update_visualizer();
    
    // Mouse durumunu güncelle
    mouse_state_t mouse_state;
    if (get_mouse_state(&mouse_state) == 0) {
        mixer_ui->mouse_x = mouse_state.x;
        mixer_ui->mouse_y = mouse_state.y;
        mixer_ui->mouse_pressed = mouse_state.buttons & 0x01;
    }
}

int audio_get_active_channels() {
    if (!audio_initialized) return 0;
    
    int count = 0;
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (audio_mixer.channels[i].active) {
            count++;
        }
    }
    
    return count;
}

void launch_audio_mixer() {
    audio_mixer_init();
    advanced_sound_init();
    
    int win_id = create_window("Ses Mixer", 5, 2, AUDIO_MIXER_WIDTH, AUDIO_MIXER_HEIGHT, 
                              (MAGENTA << 4) | BLACK);
    window_t* win = get_window(win_id);
    
    if (win) {
        win->draw_content = audio_mixer_draw_window;
        win->on_click = audio_mixer_handle_click;
        win->on_key = audio_mixer_handle_key;
        win->on_update = audio_mixer_update;
        win->data = mixer_ui;
    }
}
