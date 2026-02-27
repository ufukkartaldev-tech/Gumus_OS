#include <stdint.h>

void clear_screen() {
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) v[i] = ((uint16_t)' ') | (0x0F << 8);
}
