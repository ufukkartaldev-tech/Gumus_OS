#include "mouse.h"
#include <stdint.h>

static int mx = 160, my = 100;
static uint8_t mb = 0;

void mouse_init() {}
void mouse_wait(uint8_t type) { (void)type; }
void mouse_write(uint8_t write) { (void)write; }
uint8_t mouse_read() { return 0; }
void draw_mouse_cursor() {}
void handle_mouse_packet() {}

int get_mouse_state(mouse_state_t* state) {
    if (!state) return -1;
    state->x = mx;
    state->y = my;
    state->buttons = mb;
    return 0;
}
