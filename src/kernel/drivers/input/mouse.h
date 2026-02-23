#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init();
void mouse_handler();
void mouse_wait(uint8_t type);
void mouse_write(uint8_t write);
uint8_t mouse_read();
void draw_mouse_cursor();
void handle_mouse_packet();

// Ä°mleÃ§ pozisyonu (kernel.c'den alÄ±nacak)
// Åimdilik burada tanÄ±mlamÄ±yoruz, kernel.c iÃ§inde extern yapacaÄŸÄ±z

typedef struct {
    int x;
    int y;
    uint8_t buttons;
} mouse_state_t;

int get_mouse_state(mouse_state_t* state);

#endif
