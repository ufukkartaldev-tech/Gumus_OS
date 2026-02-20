#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>

// Nota Frekansları (Hz) - 4. Oktav
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

// Nota Yapısı {frekans, sure_tik}
typedef struct {
    uint32_t frequency;
    uint32_t duration;
} note_t;

void beep();
void play_sound(uint32_t nFrequence);
void nosound();
void handle_beep_timer();

// Müzik fonksiyonları
void play_melody(note_t* melody, int length);
void stop_melody();

#endif
