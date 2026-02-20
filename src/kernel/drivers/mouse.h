#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init();
void mouse_handler();
void mouse_wait(uint8_t type);
void mouse_write(uint8_t write);
uint8_t mouse_read();

// İmleç pozisyonu (kernel.c'den alınacak)
// Şimdilik burada tanımlamıyoruz, kernel.c içinde extern yapacağız

#endif
