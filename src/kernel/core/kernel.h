#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include "idt.h"

void panic(const char* message, registers_t* r);
void putchar(char c);
char kbd_get();
void print(const char* str);
void print_color(const char* str, uint8_t color);
void clear_screen();
void update_status_bar();
void draw_logo();
void draw_window(int x, int y, int w, int h, const char* title, uint8_t color);

void timer_handler();
uint32_t get_timer_ticks();
void msleep(uint32_t ms);
void beep_hz(uint32_t hz, uint32_t ms);
void beep();

// VGA Renkleri
#define BLACK         0
#define BLUE          1
#define GREEN         2
#define CYAN          3
#define RED           4
#define MAGENTA       5
#define BROWN         6
#define LIGHT_GREY    7
#define DARK_GREY     8
#define LIGHT_BLUE    9
#define LIGHT_GREEN   10
#define LIGHT_CYAN    11
#define LIGHT_RED     12
#define LIGHT_MAGENTA 13
#define YELLOW        14
#define WHITE         15

#endif
