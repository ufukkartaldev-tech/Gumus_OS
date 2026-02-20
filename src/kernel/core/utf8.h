#ifndef UTF8_H
#define UTF8_H

#include <stdint.h>

// UTF-8 durumları
typedef enum {
    UTF8_ACCEPT = 0,
    UTF8_REJECT = 1
} utf8_state_t;

// UTF-8 kod çözücü yapısı
typedef struct {
    uint32_t codepoint;
    uint32_t state;
} utf8_decoder_t;

// Tek bir baytı işler ve durumu günceller
uint32_t decode_utf8(utf8_decoder_t* decoder, uint8_t byte);

// Unicode kod noktasını VGA CP437 karakterine eşler (En yakın eşleşme)
uint8_t unicode_to_cp437(uint32_t codepoint);

#endif
