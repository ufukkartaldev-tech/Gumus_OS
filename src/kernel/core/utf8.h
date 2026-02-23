#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

// UTF-8 durumlarÄ±
typedef enum {
    UTF8_ACCEPT = 0,
    UTF8_REJECT = 1
} utf8_state_t;

// UTF-8 kod Ã§Ã¶zÃ¼cÃ¼ yapÄ±sÄ±
typedef struct {
    uint32_t codepoint;
    uint32_t state;
} utf8_decoder_t;

// Tek bir baytÄ± iÅŸler ve durumu gÃ¼nceller
uint32_t decode_utf8(utf8_decoder_t* decoder, uint8_t byte);

// Unicode kod noktasÄ±nÄ± VGA CP437 karakterine eÅŸler (En yakÄ±n eÅŸleÅŸme)
uint8_t unicode_to_cp437(uint32_t codepoint);

#endif
