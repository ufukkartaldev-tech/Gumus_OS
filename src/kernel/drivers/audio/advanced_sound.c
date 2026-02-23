#include "advanced_sound.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "fs.h"
#include "ata.h"
#include "printf.h"
#include "math.h"

// Global deÄŸiÅŸkenler
static audio_mixer_t audio_mixer;
static int audio_initialized = 0;
static int next_channel_id = 0;

// PC Speaker port'larÄ±
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43
#define SPEAKER_PORT 0x61

// Timer frekansÄ±
#define TIMER_FREQUENCY 1193180

void advanced_sound_init() {
    if (audio_initialized) return;
    
    memset(&audio_mixer, 0, sizeof(audio_mixer));
    
    // VarsayÄ±lan ayarlar
    audio_mixer.master_volume = 50;
    audio_mixer.left_volume = 50;
    audio_mixer.right_volume = 50;
    
    // TÃ¼m kanallarÄ± pasif yap
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        audio_mixer.channels[i].active = 0;
        audio_mixer.channels[i].volume = 100;
        audio_mixer.channels[i].pan = 128; // Center
    }
    
    audio_initialized = 1;
    printf("GeliÅŸmiÅŸ Ses Sistemi baÅŸlatÄ±ldÄ±\n");
}

void advanced_sound_shutdown() {
    if (!audio_initialized) return;
    
    // TÃ¼m kanallarÄ± durdur
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        audio_stop_channel(i);
    }
    
    // PC Speaker'Ä± kapat
    nosound();
    
    audio_initialized = 0;
    printf("Ses sistemi kapatÄ±ldÄ±\n");
}

int audio_create_channel() {
    if (!audio_initialized) return -1;
    
    // BoÅŸ kanal bul
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (!audio_mixer.channels[i].active) {
            audio_mixer.channels[i].active = 1;
            audio_mixer.channels[i].buffer = NULL;
            audio_mixer.channels[i].buffer_size = 0;
            audio_mixer.channels[i].position = 0;
            audio_mixer.channels[i].sample_rate = AUDIO_SAMPLE_RATE;
            audio_mixer.channels[i].format = AUDIO_FORMAT_PCM8;
            audio_mixer.channels[i].volume = 100;
            audio_mixer.channels[i].pan = 128;
            audio_mixer.channels[i].loop = 0;
            
            return i;
        }
    }
    
    return -1; // BoÅŸ kanal yok
}

void audio_destroy_channel(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    
    if (ch->buffer) {
        kfree(ch->buffer);
        ch->buffer = NULL;
    }
    
    ch->active = 0;
}

int audio_play_buffer(int channel, uint8_t* buffer, uint32_t size, audio_format_t format) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return -1;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    
    // Eski buffer'Ä± temizle
    if (ch->buffer) {
        kfree(ch->buffer);
    }
    
    // Yeni buffer'Ä± ayÄ±r
    ch->buffer = (uint8_t*)kmalloc(size);
    if (!ch->buffer) return -1;
    
    memcpy(ch->buffer, buffer, size);
    ch->buffer_size = size;
    ch->position = 0;
    ch->format = format;
    ch->active = 1;
    
    printf("Ses buffer'Ä± kanal %d'de Ã§alÄ±nÄ±yor (%d bytes)\n", channel, size);
    return 0;
}

void audio_stop_channel(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    ch->active = 0;
    ch->position = 0;
    
    if (channel == 0) {
        // Ana kanalÄ± PC Speaker'da durdur
        nosound();
    }
}

void audio_pause_channel(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    ch->active = 0;
}

void audio_resume_channel(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    if (ch->buffer && ch->buffer_size > 0) {
        ch->active = 1;
    }
}

void audio_set_volume(int channel, uint8_t volume) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_mixer.channels[channel].volume = volume;
}

void audio_set_pan(int channel, uint8_t pan) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_mixer.channels[channel].pan = pan;
}

void audio_set_master_volume(uint8_t volume) {
    if (!audio_initialized) return;
    
    audio_mixer.master_volume = volume;
}

void audio_set_loop(int channel, uint8_t loop) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    
    audio_mixer.channels[channel].loop = loop;
}

int audio_load_wav(const char* filename, uint8_t** buffer, uint32_t* size, wav_header_t* header) {
    if (!audio_initialized || !filename || !buffer || !size || !header) return -1;
    
    // DosyayÄ± oku
    uint8_t* file_data = fs_read(filename);
    if (!file_data) {
        printf("WAV dosyasÄ± okunamadÄ±: %s\n", filename);
        return -1;
    }
    
    // WAV baÅŸlÄ±ÄŸÄ±nÄ± kontrol et
    wav_header_t* wav_hdr = (wav_header_t*)file_data;
    
    if (strncmp(wav_hdr->riff, "RIFF", 4) != 0 || 
        strncmp(wav_hdr->wave, "WAVE", 4) != 0) {
        printf("GeÃ§ersiz WAV formatÄ±: %s\n", filename);
        kfree(file_data);
        return -1;
    }
    
    // Header'Ä± kopyala
    memcpy(header, wav_hdr, sizeof(wav_header_t));
    
    // Ses verisini ayÄ±r
    uint32_t data_size = header->data_size;
    *buffer = (uint8_t*)kmalloc(data_size);
    if (!*buffer) {
        kfree(file_data);
        return -1;
    }
    
    // Ses verisini kopyala
    memcpy(*buffer, file_data + sizeof(wav_header_t), data_size);
    *size = data_size;
    
    kfree(file_data);
    
    printf("WAV dosyasÄ± yÃ¼klendi: %s (%d Hz, %d kanal, %d-bit, %d bytes)\n", 
           filename, header->sample_rate, header->channels, 
           header->bits_per_sample, data_size);
    
    return 0;
}

int audio_save_wav(const char* filename, uint8_t* buffer, uint32_t size, wav_header_t* header) {
    if (!audio_initialized || !filename || !buffer || !header) return -1;
    
    // Dosya boyutunu hesapla
    uint32_t file_size = sizeof(wav_header_t) + size;
    
    // Header'Ä± gÃ¼ncelle
    header->file_size = file_size - 8;
    header->data_size = size;
    
    // Tam dosyayÄ± oluÅŸtur
    uint8_t* file_data = (uint8_t*)kmalloc(file_size);
    if (!file_data) return -1;
    
    // Header ve veriyi kopyala
    memcpy(file_data, header, sizeof(wav_header_t));
    memcpy(file_data + sizeof(wav_header_t), buffer, size);
    
    // Dosyaya yaz
    fs_write(filename, (char*)file_data);
    
    kfree(file_data);
    
    printf("WAV dosyasÄ± kaydedildi: %s (%d bytes)\n", filename, size);
    return 0;
}

void audio_play_midi_note(midi_note_t* note) {
    if (!audio_initialized || !note) return;
    
    // MIDI nota frekansÄ±na Ã§evir
    uint32_t frequency = 440; // A4 = 440Hz
    
    if (note->note >= 0 && note->note <= 127) {
        // Basit nota-frekans dÃ¶nÃ¼ÅŸÃ¼mÃ¼
        frequency = 440 * pow(2, (note->note - 69) / 12.0);
    }
    
    // Kanalda Ã§al
    int channel = audio_create_channel();
    if (channel >= 0) {
        // Basit ses buffer'Ä± oluÅŸtur
        uint8_t* tone_buffer = (uint8_t*)kmalloc(1000);
        
        // Basit sinÃ¼s dalgasÄ± oluÅŸtur (sadeleÅŸmiÅŸ)
        for (int i = 0; i < 1000; i++) {
            float angle = 2.0 * 3.14159 * i * frequency / AUDIO_SAMPLE_RATE;
            tone_buffer[i] = 128 + 127 * sin(angle) * (note->velocity / 127.0);
        }
        
        audio_play_buffer(channel, tone_buffer, 1000, AUDIO_FORMAT_PCM8);
        audio_set_volume(channel, note->velocity * 100 / 127);
        
        // Belirtilen sÃ¼re sonra durdur
        // GerÃ§ek bir sistemde timer ile yapÄ±lmalÄ±
        if (note->duration > 0) {
            // Åimdilik sadece baÅŸlatÄ±yoruz
        }
        
        kfree(tone_buffer);
    }
}

void audio_stop_midi_note(uint8_t note, uint8_t channel) {
    if (!audio_initialized) return;
    
    // Belirtilen kanaldaki notayÄ± durdur
    // GerÃ§ek bir sistemde polyphony yÃ¶netimi gerekir
    printf("MIDI nota durduruldu: %d (kanal %d)\n", note, channel);
}

void audio_add_effect(audio_effect_params_t* effect) {
    if (!audio_initialized || !effect || audio_mixer.effect_count >= 4) return;
    
    memcpy(&audio_mixer.effects[audio_mixer.effect_count], effect, sizeof(audio_effect_params_t));
    audio_mixer.effect_count++;
    
    printf("Ses efekti eklendi: %d\n", effect->type);
}

void audio_remove_effect(int index) {
    if (!audio_initialized || index < 0 || index >= audio_mixer.effect_count) return;
    
    // Efekti sil ve diÄŸerlerini kaydÄ±r
    for (int i = index; i < audio_mixer.effect_count - 1; i++) {
        memcpy(&audio_mixer.effects[i], &audio_mixer.effects[i + 1], sizeof(audio_effect_params_t));
    }
    
    audio_mixer.effect_count--;
}

void audio_clear_effects() {
    if (!audio_initialized) return;
    
    audio_mixer.effect_count = 0;
}

void audio_mix_samples() {
    if (!audio_initialized) return;
    
    // Mix buffer'Ä±nÄ± temizle
    memset(audio_mixer.mix_buffer, 0, sizeof(audio_mixer.mix_buffer));
    
    // Aktif kanallarÄ± karÄ±ÅŸtÄ±r
    for (int ch = 0; ch < AUDIO_MAX_CHANNELS; ch++) {
        audio_channel_t* channel = &audio_mixer.channels[ch];
        
        if (!channel->active || !channel->buffer) continue;
        
        // Kanal sesini mix buffer'Ä±na ekle
        for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
            if (channel->position >= channel->buffer_size) {
                if (channel->loop) {
                    channel->position = 0;
                } else {
                    channel->active = 0;
                    break;
                }
            }
            
            if (channel->active) {
                int16_t sample = 0;
                
                // Formata gÃ¶re sample'Ä± oku
                if (channel->format == AUDIO_FORMAT_PCM8) {
                    sample = ((int16_t)channel->buffer[channel->position] - 128) * 256;
                } else if (channel->format == AUDIO_FORMAT_PCM16) {
                    sample = *(int16_t*)&channel->buffer[channel->position];
                }
                
                // Ses seviyesi ve pan
                float volume_factor = (channel->volume / 100.0) * (audio_mixer.master_volume / 100.0);
                float pan_left = 1.0, pan_right = 1.0;
                
                if (channel->pan < 128) {
                    pan_left = 1.0;
                    pan_right = channel->pan / 128.0;
                } else {
                    pan_left = (255 - channel->pan) / 128.0;
                    pan_right = 1.0;
                }
                
                // Mix'e ekle
                audio_mixer.mix_buffer[i] += (int16_t)(sample * volume_factor * pan_left);
                
                channel->position++;
            }
        }
    }
    
    // Efektleri uygula
    audio_apply_effects();
}

void audio_apply_effects() {
    if (!audio_initialized || audio_mixer.effect_count == 0) return;
    
    // Mix buffer'Ä± output buffer'Ä±na kopyala
    memcpy(audio_mixer.output_buffer, audio_mixer.mix_buffer, sizeof(audio_mixer.output_buffer));
    
    // Efektleri uygula
    for (int e = 0; e < audio_mixer.effect_count; e++) {
        audio_effect_params_t* effect = &audio_mixer.effects[e];
        
        switch (effect->type) {
            case EFFECT_ECHO:
                // Basit echo efekti
                for (int i = effect->delay; i < AUDIO_BUFFER_SIZE; i++) {
                    audio_mixer.output_buffer[i] += (int16_t)(audio_mixer.output_buffer[i - effect->delay] * effect->intensity);
                }
                break;
                
            case EFFECT_REVERB:
                // Basit reverb efekti
                for (int i = 1; i < AUDIO_BUFFER_SIZE; i++) {
                    audio_mixer.output_buffer[i] += (int16_t)(audio_mixer.output_buffer[i-1] * effect->intensity * 0.5);
                }
                break;
                
            case EFFECT_DISTORTION:
                // Basit distortion efekti
                for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                    int16_t sample = audio_mixer.output_buffer[i];
                    if (sample > 20000) sample = 20000;
                    if (sample < -20000) sample = -20000;
                    audio_mixer.output_buffer[i] = sample;
                }
                break;
                
            default:
                break;
        }
    }
}

void audio_update() {
    if (!audio_initialized) return;
    
    // Ses karÄ±ÅŸtÄ±rma
    audio_mix_samples();
    
    // PC Speaker'a gÃ¶nder (basitleÅŸtirilmiÅŸ)
    if (audio_mixer.channels[0].active && audio_mixer.channels[0].buffer) {
        // Ä°lk sample'Ä± al ve frekansa Ã§evir
        uint8_t sample = audio_mixer.output_buffer[0] >> 8; // 16-bit'ten 8-bite
        uint32_t frequency = 200 + (sample * 2); // Basit frekans dÃ¶nÃ¼ÅŸÃ¼mÃ¼
        
        play_sound(frequency);
    } else {
        nosound();
    }
}

uint32_t audio_get_duration(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return 0;
    
    audio_channel_t* ch = &audio_mixer.channels[channel];
    
    if (!ch->active || !ch->buffer) return 0;
    
    // SÃ¼reyi hesapla (saniye)
    return (ch->buffer_size * 1000) / ch->sample_rate;
}

int audio_is_playing(int channel) {
    if (!audio_initialized || channel < 0 || channel >= AUDIO_MAX_CHANNELS) return 0;
    
    return audio_mixer.channels[channel].active;
}

float audio_get_cpu_usage() {
    if (!audio_initialized) return 0.0;
    
    int active_channels = 0;
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (audio_mixer.channels[i].active) {
            active_channels++;
        }
    }
    
    return (active_channels * 100.0) / AUDIO_MAX_CHANNELS;
}

// GerÃ§ek zamanlÄ± ses gÃ¼ncelleme (timer interrupt'Ä±nda Ã§aÄŸrÄ±lÄ±r)
void audio_timer_handler() {
    if (!audio_initialized) return;
    
    audio_update();
}

// Ses kayÄ±t sistemi (basit implementasyon)
static uint8_t* recording_buffer = NULL;
static uint32_t recording_size = 0;
static int is_recording = 0;

void audio_start_recording() {
    if (!audio_initialized) return;
    
    if (recording_buffer) {
        kfree(recording_buffer);
    }
    
    recording_buffer = (uint8_t*)kmalloc(AUDIO_BUFFER_SIZE * 100); // 100 buffer
    recording_size = 0;
    is_recording = 1;
    
    printf("Ses kaydÄ± baÅŸlatÄ±ldÄ±\n");
}

void audio_stop_recording() {
    if (!audio_initialized || !is_recording) return;
    
    is_recording = 0;
    printf("Ses kaydÄ± durduruldu (%d bytes)\n", recording_size);
}

void audio_save_recording(const char* filename) {
    if (!audio_initialized || !recording_buffer || recording_size == 0) return;
    
    // WAV baÅŸlÄ±ÄŸÄ± oluÅŸtur
    wav_header_t header;
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);
    
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.channels = 1; // Mono
    header.sample_rate = AUDIO_SAMPLE_RATE;
    header.bits_per_sample = 8;
    header.block_align = header.channels * header.bits_per_sample / 8;
    header.byte_rate = header.sample_rate * header.block_align;
    header.data_size = recording_size;
    
    audio_save_wav(filename, recording_buffer, recording_size, &header);
}

int audio_is_recording() {
    return is_recording;
}
