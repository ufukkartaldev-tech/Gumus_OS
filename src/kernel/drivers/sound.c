#include "sound.h"
#include "io.h"

static volatile uint32_t beep_ticks = 0;

// Arka plan müzik değişkenleri
static note_t* current_melody = 0;
static int current_melody_len = 0;
static int current_note_idx = 0;
static int note_ticks_left = 0;
static int is_playing = 0;

// PC Hoparlörünü Çalma
void play_sound(uint32_t nFrequence) {
    if (nFrequence == 0) return; // Sıfıra bölme hatasını önle

    uint32_t Div;
    uint8_t tmp;

    // Frekans sayacını ayarla (1193180 / Frekans)
    Div = 1193180 / nFrequence;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t) (Div) );
    outb(0x42, (uint8_t) (Div >> 8));

    // Hoparlörü aç (Gereksiz kontrol kaldırıldı)
    tmp = inb(0x61);
    outb(0x61, tmp | 3);
}

// Hoparlörü Kapatma
void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

// BEEP sesi (Non-blocking / Bloklamayan)
void beep() {
    play_sound(1000); // 1000 Hz
    beep_ticks = 3;   // Yaklaşık 150ms (18.2 Hz timer'da 3 tık)
}

// Timer kesmesi tarafından çağrılacak
// Müzik Çalma Başlatma
void play_melody(note_t* melody, int length) {
    if (length <= 0) return;
    
    // Müzik parametrelerini ayarla
    current_melody = melody;
    current_melody_len = length;
    current_note_idx = 0;
    
    // İlk notayı çal
    play_sound(current_melody[0].frequency);
    note_ticks_left = current_melody[0].duration;
    
    is_playing = 1;
}

// Müziği Durdurma
void stop_melody() {
    is_playing = 0;
    nosound();
    current_melody = 0;
    current_melody_len = 0;
}

// Timer kesmesi tarafından çağrılacak (beep ve müzik için)
void handle_beep_timer() {
    // 1. Beep Yönetimi (Öncelikli)
    if (beep_ticks > 0) {
        beep_ticks--;
        if (beep_ticks == 0) {
            // Eğer müzik çalıyorsa beep bitince susmasın, müziğe dönsün
            if (!is_playing) {
                nosound();
            } else {
                // Müziğe geri dön (Kaldığı yerden)
                if (current_melody && current_note_idx < current_melody_len) {
                    play_sound(current_melody[current_note_idx].frequency);
                }
            }
        }
        return; // Beep sırasınca müzik sayacı işlemesin
    }

    // 2. Müzik Yönetimi
    if (is_playing && current_melody) {
        if (note_ticks_left > 0) {
            note_ticks_left--;
        } else {
            // Nota bitti, sonraki notaya geç
            current_note_idx++;
            
            if (current_note_idx >= current_melody_len) {
                // Şarkı bitti
                stop_melody();
            } else {
                // Yeni notayı çal
                uint32_t freq = current_melody[current_note_idx].frequency;
                uint32_t dur = current_melody[current_note_idx].duration;
                
                if (freq == 0) nosound(); // Es (Sessizlik)
                else play_sound(freq);
                
                note_ticks_left = dur;
            }
        }
    }
}
