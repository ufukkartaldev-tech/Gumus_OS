#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// TÃ¼rkÃ§e Q Klavye HaritasÄ± (Basit Karakterler)
// Not: UTF-8 karakterler (ÄŸ, ÅŸ vb.) burada 0 olarak iÅŸaretlenip 
// kernel tarafÄ±nda Ã¶zel iÅŸlenecektir.
unsigned char keyboard_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '*', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', 0, 0, '\n',	/* Enter key, 26, 27: Ä, Ãœ */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0,	/* 39 - 39: Å */
  'i', '\"',   0,		/* 40: Ä°, 41: Left shift */
  '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', 0, 0, '.',   0,				/* 51: Ã–, 52: Ã‡ */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0x80,	/* Up Arrow (Ã–zel Kod) */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0x81,	/* Down Arrow (Ã–zel Kod) */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

// Keyboard handler function
void handle_keyboard(uint8_t scancode);

#endif
