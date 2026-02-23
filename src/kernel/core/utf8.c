#include "utf8.h"

// UTF-8 State Machine (Bjoern Hoehrmann's implementation style)
static const uint8_t utf8d[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
    0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
    0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
    0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
    2,1,2,2,1,2,2,2,1,2,2,2,1,1,1,1,3,3,3,1,3,3,3,3,3,3,3,3,3,3,3,3, // s3..s4
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s5..s6
};

uint32_t decode_utf8(utf8_decoder_t* decoder, uint8_t byte) {
    uint32_t type = utf8d[byte];
    decoder->codepoint = (decoder->state != UTF8_ACCEPT) ?
        (byte & 0x3fu) | (decoder->codepoint << 6) :
        (0xff >> type) & byte;
    decoder->state = utf8d[256 + decoder->state * 16 + type];
    return decoder->state;
}

uint8_t unicode_to_cp437(uint32_t codepoint) {
    if (codepoint < 128) return (uint8_t)codepoint;

    // TÃ¼rkÃ§e Karakter EÅŸlemeleri
    switch (codepoint) {
        case 0x00C7: return 0x80; // Ã‡
        case 0x00E7: return 0x87; // Ã§
        case 0x011E: return 0xA7; // Ä (Yeni font)
        case 0x011F: return 0xA6; // ÄŸ (Yeni font)
        case 0x0130: return 0xAB; // Ä° (Yeni font)
        case 0x0131: return 0xAA; // Ä± (Yeni font)
        case 0x00D6: return 0x99; // Ã–
        case 0x00F6: return 0x94; // Ã¶
        case 0x015E: return 0xA9; // Å (Yeni font)
        case 0x015F: return 0xA8; // ÅŸ (Yeni font)
        case 0x00DC: return 0x9A; // Ãœ
        case 0x00FC: return 0x81; // Ã¼
        default: return '?';      // Bilinmeyen karakter
    }
}
