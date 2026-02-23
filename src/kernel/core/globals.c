#include <stddef.h>
#include <stdint.h>
#include "vga_gfx.h"
#include "advanced_sound.h"
#include "fs.h"

// Global variables
uint8_t* backbuffer = NULL;

void* get_vram() {
    // Simple VRAM pointer - in real system this would return actual VRAM address
    return (void*)0xA0000; // VGA memory address
}

// Audio globals
audio_mixer_t audio_mixer;
int audio_initialized = 0;

// File system globals
uint32_t used_blocks = 0;
uint32_t max_blocks = 1024;

// Timer function
uint32_t get_tick_count() {
    // Simple tick counter - in real system this would read hardware timer
    static uint32_t ticks = 0;
    return ++ticks;
}
