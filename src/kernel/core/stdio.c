// Basit VGA metin modu çıkışı
#include <stdint.h>
#include "string.h"

static int _cx = 0, _cy = 0;
static const int _cols = 80, _rows = 25;

static void _scroll_if_needed() {
    if (_cy < _rows) return;
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    for (int i = 1; i < _rows; i++) {
        for (int j = 0; j < _cols; j++) {
            v[(i-1)*_cols + j] = v[i*_cols + j];
        }
    }
    for (int j = 0; j < _cols; j++) v[(_rows-1)*_cols + j] = (uint16_t)' ' | (0x0F << 8);
    _cy = _rows - 1;
}

void putchar(char c) {
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    if (c == '\n') {
        _cx = 0; _cy++;
        _scroll_if_needed();
        return;
    }
    if (_cx >= _cols) { _cx = 0; _cy++; _scroll_if_needed(); }
    v[_cy * _cols + _cx] = ((uint16_t)c) | (0x0F << 8);
    _cx++;
}

void print(const char* s) {
    for (int i = 0; s[i]; i++) putchar(s[i]);
}

void print_color(const char* s, uint8_t color) {
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    for (int i = 0; s[i]; i++) {
        if (s[i] == '\n') { _cx = 0; _cy++; _scroll_if_needed(); continue; }
        if (_cx >= _cols) { _cx = 0; _cy++; _scroll_if_needed(); }
        v[_cy * _cols + _cx] = ((uint16_t)s[i]) | ((uint16_t)color << 8);
        _cx++;
    }
}
