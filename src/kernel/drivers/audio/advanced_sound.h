#ifndef ADVANCED_SOUND_H
#define ADVANCED_SOUND_H

#include <stdint.h>
#include "sound.h"

// GeliÅŸmiÅŸ Ses Sistemi Sabitleri
#define AUDIO_MAX_CHANNELS 8
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_MAX_VOLUME 100

// Ses FormatlarÄ±
typedef enum {
    AUDIO_FORMAT_PCM8,
    AUDIO_FORMAT_PCM16,
    AUDIO_FORMAT_MIDI,
    AUDIO_FORMAT_WAV
} audio_format_t;

// Ses KanalÄ±
typedef struct {
    int active;
    uint8_t* buffer;
    uint32_t buffer_size;
    uint32_t position;
    uint32_t sample_rate;
    audio_format_t format;
    uint8_t volume;
    uint8_t pan; // 0=left, 128=center, 255=right
    uint8_t loop;
} audio_channel_t;

// Ses Efektleri
typedef enum {
    EFFECT_NONE,
    EFFECT_ECHO,
    EFFECT_REVERB,
    EFFECT_CHORUS,
    EFFECT_DISTORTION
} audio_effect_t;

// Efekt Parametreleri
typedef struct {
    audio_effect_t type;
    float intensity;
    uint32_t delay;
    uint32_t feedback;
} audio_effect_params_t;

// Ses KarÄ±ÅŸtÄ±rÄ±cÄ± (Mixer)
typedef struct {
    audio_channel_t channels[AUDIO_MAX_CHANNELS];
    uint8_t master_volume;
    uint8_t left_volume;
    uint8_t right_volume;
    
    // Efektler
    audio_effect_params_t effects[4];
    int effect_count;
    
    // Buffer'lar
    int16_t mix_buffer[AUDIO_BUFFER_SIZE];
    int16_t output_buffer[AUDIO_BUFFER_SIZE];
} audio_mixer_t;

// WAV Dosya BaÅŸlÄ±ÄŸÄ±
typedef struct {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} __attribute__((packed)) wav_header_t;

// MIDI Nota YapÄ±sÄ±
typedef struct {
    uint8_t note;
    uint8_t velocity;
    uint32_t duration;
    uint8_t channel;
} midi_note_t;

// GeliÅŸmiÅŸ Ses FonksiyonlarÄ±
void advanced_sound_init();
void advanced_sound_shutdown();

// Kanal YÃ¶netimi
int audio_create_channel();
void audio_destroy_channel(int channel);
int audio_play_buffer(int channel, uint8_t* buffer, uint32_t size, audio_format_t format);
int audio_play_file(int channel, const char* filename);
void audio_stop_channel(int channel);
void audio_pause_channel(int channel);
void audio_resume_channel(int channel);

// Ses Kontrolleri
void audio_set_volume(int channel, uint8_t volume);
void audio_set_pan(int channel, uint8_t pan);
void audio_set_master_volume(uint8_t volume);
void audio_set_loop(int channel, uint8_t loop);
int audio_get_active_channels();

// Efektler
void audio_add_effect(audio_effect_params_t* effect);
void audio_remove_effect(int index);
void audio_clear_effects();

// WAV FormatÄ±
int audio_load_wav(const char* filename, uint8_t** buffer, uint32_t* size, wav_header_t* header);
int audio_save_wav(const char* filename, uint8_t* buffer, uint32_t size, wav_header_t* header);

// MIDI FormatÄ±
void audio_play_midi_note(midi_note_t* note);
void audio_stop_midi_note(uint8_t note, uint8_t channel);
void audio_load_midi_file(const char* filename);

// Ses KayÄ±t
void audio_start_recording();
void audio_stop_recording();
void audio_save_recording(const char* filename);
int audio_is_recording();

// GerÃ§ek ZamanlÄ± Ä°ÅŸleme
void audio_update();
void audio_mix_samples();
void audio_apply_effects();

// Utility FonksiyonlarÄ±
uint32_t audio_get_duration(int channel);
int audio_is_playing(int channel);
float audio_get_cpu_usage();

// Ses ArayÃ¼zÃ¼ FonksiyonlarÄ±
void audio_show_visualizer();
void audio_show_equalizer();
void audio_show_mixer();

#endif
