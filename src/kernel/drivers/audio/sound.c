#include "sound.h"
#include "io.h"

static volatile uint32_t beep_ticks = 0;

// Arka plan mÃ¼zik deÄŸiÅŸkenleri
static note_t* current_melody = 0;
static int current_melody_len = 0;
static int current_note_idx = 0;
static int note_ticks_left = 0;
static int is_playing = 0;

// PC HoparlÃ¶rÃ¼nÃ¼ Ã‡alma
void play_sound(uint32_t nFrequence) {
    if (nFrequence == 0) return; // SÄ±fÄ±ra bÃ¶lme hatasÄ±nÄ± Ã¶nle

    uint32_t Div;
    uint8_t tmp;

    // Frekans sayacÄ±nÄ± ayarla (1193180 / Frekans)
    Div = 1193180 / nFrequence;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t) (Div) );
    outb(0x42, (uint8_t) (Div >> 8));

    // HoparlÃ¶rÃ¼ aÃ§ (Gereksiz kontrol kaldÄ±rÄ±ldÄ±)
    tmp = inb(0x61);
    outb(0x61, tmp | 3);
}

// HoparlÃ¶rÃ¼ Kapatma
void nosound() {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

// BEEP sesi (Non-blocking / Bloklamayan)
void beep() {
    play_sound(1000); // 1000 Hz
    beep_ticks = 3;   // YaklaÅŸÄ±k 150ms (18.2 Hz timer'da 3 tÄ±k)
}

// Timer kesmesi tarafÄ±ndan Ã§aÄŸrÄ±lacak
// MÃ¼zik Ã‡alma BaÅŸlatma
void play_melody(note_t* melody, int length) {
    if (length <= 0) return;
    
    // MÃ¼zik parametrelerini ayarla
    current_melody = melody;
    current_melody_len = length;
    current_note_idx = 0;
    
    // Ä°lk notayÄ± Ã§al
    play_sound(current_melody[0].frequency);
    note_ticks_left = current_melody[0].duration;
    
    is_playing = 1;
}

// MÃ¼ziÄŸi Durdurma
void stop_melody() {
    is_playing = 0;
    nosound();
    current_melody = 0;
    current_melody_len = 0;
}

// Timer kesmesi tarafÄ±ndan Ã§aÄŸrÄ±lacak (beep ve mÃ¼zik iÃ§in)
void handle_beep_timer() {
    // 1. Beep YÃ¶netimi (Ã–ncelikli)
    if (beep_ticks > 0) {
        beep_ticks--;
        if (beep_ticks == 0) {
            // EÄŸer mÃ¼zik Ã§alÄ±yorsa beep bitince susmasÄ±n, mÃ¼ziÄŸe dÃ¶nsÃ¼n
            if (!is_playing) {
                nosound();
            } else {
                // MÃ¼ziÄŸe geri dÃ¶n (KaldÄ±ÄŸÄ± yerden)
                if (current_melody && current_note_idx < current_melody_len) {
                    play_sound(current_melody[current_note_idx].frequency);
                }
            }
        }
        return; // Beep sÄ±rasÄ±nca mÃ¼zik sayacÄ± iÅŸlemesin
    }

    // 2. MÃ¼zik YÃ¶netimi
    if (is_playing && current_melody) {
        if (note_ticks_left > 0) {
            note_ticks_left--;
        } else {
            // Nota bitti, sonraki notaya geÃ§
            current_note_idx++;
            
            if (current_note_idx >= current_melody_len) {
                // ÅarkÄ± bitti
                stop_melody();
            } else {
                // Yeni notayÄ± Ã§al
                uint32_t freq = current_melody[current_note_idx].frequency;
                uint32_t dur = current_melody[current_note_idx].duration;
                
                if (freq == 0) nosound(); // Es (Sessizlik)
                else play_sound(freq);
                
                note_ticks_left = dur;
            }
        }
    }
}
