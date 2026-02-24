#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include <stdint.h>
#include <stddef.h>

/**
 * GümüşOS Test Framework (Zırhlı Versiyon v2)
 * -----------------------------------------
 * Dayı Tavsiyesi 1: Statik değişkenlerin çakışmaması için inline veya global kullanılır.
 * Testler parça parça derlendiği için her test dosyasında kendine özel _cy olması 
 * izole testler için (şu anki sistemimiz) mantıklıdır. 
 * Ama Higher-Half için vba_print_raw adresleri güncellendi.
 */

#define TEST_PASS 1
#define TEST_FAIL 0

static int _cy = 4;

// Dayı Tavsiyesi 2: volatile uint16_t* (Derleyici optimizasyonuna karşı zırh)
static void _print_raw(const char* s, int x, int y, uint8_t c) {
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    for (int i = 0; s[i]; i++) {
        v[y * 80 + x + i] = (uint16_t)s[i] | ((uint16_t)c << 8);
    }
}

// Dayı Tavsiyesi 3: Basit Kaydırma (Scrolling) Mantığı
static void _scroll_check() {
    if (_cy > 24) {
        volatile uint16_t* v = (volatile uint16_t*)0xB8000;
        // 4. satırdan sonrasını yukarı kaydır (Header'ı koru)
        for (int i = 4 * 80; i < 24 * 80; i++) {
            v[i] = v[i + 80];
        }
        // Son satırı temizle
        for (int i = 24 * 80; i < 25 * 80; i++) {
            v[i] = (uint16_t)' ' | (0x0F << 8);
        }
        _cy = 24;
    }
}

static void test_header(const char* t) {
    volatile uint16_t* v = (volatile uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) v[i] = (uint16_t)' ' | (0x0F << 8);
    _print_raw("--- GUMUS OS RIGOROUS TEST SUITE ---", 22, 1, 0x0B);
    _print_raw(t, 2, 2, 0x0E);
    _cy = 4;
}

// Dayı Tavsiyesi 4: Hex prefix (0x) ve daha düzenli itoa
static void _test_itoa_hex(uint32_t n, char* s) {
    char* p = s;
    *p++ = '0'; *p++ = 'x'; 
    char* start = p;
    if (n == 0) *p++ = '0';
    while (n > 0) {
        uint32_t tmp = n % 16;
        *p++ = (tmp < 10) ? (tmp + '0') : (tmp - 10 + 'A');
        n /= 16;
    }
    *p = '\0';
    // Reverse hex part
    char* q = p - 1;
    while (start < q) {
        char t = *start;
        *start++ = *q;
        *q-- = t;
    }
}

// Dayı Tavsiyesi 5: Psikolojik Renkler (Exp: Yeşil, Got: Kırmızı)
static void ASSERT_DETAIL(int cond, const char* msg, uint32_t expected, uint32_t actual) {
    if (!cond) {
        _scroll_check();
        _print_raw("[FAIL]", 2, _cy, 0x0C);
        _print_raw(msg, 9, _cy, 0x0F);
        
        char buf[32];
        _print_raw("Exp:", 42, _cy, 0x07);
        _test_itoa_hex(expected, buf); 
        _print_raw(buf, 47, _cy, 0x0A); // YEŞİL (Beklenen doğrudur)
        
        _print_raw("Got:", 60, _cy, 0x07);
        _test_itoa_hex(actual, buf);
        _print_raw(buf, 65, _cy, 0x0C); // KIRMIZI (Gelen yanlıştır)
        
        _cy++;
    }
}

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            _scroll_check(); \
            _print_raw("[FAIL]", 2, _cy, 0x0C); \
            _print_raw(msg, 9, _cy, 0x0F); \
            _cy++; return TEST_FAIL; \
        } \
    } while (0)

// "Zırhlı" Mühürlü Makro (Double Evaluation Koruması)
#define ASSERT_EQ(actual, expected, msg) \
    do { \
        uint32_t _act = (uint32_t)(actual); \
        uint32_t _exp = (uint32_t)(expected); \
        if (_act != _exp) { \
            ASSERT_DETAIL(0, msg, _exp, _act); \
            return TEST_FAIL; \
        } \
    } while (0)

#define RUN_TEST(func, name) \
    do { \
        _scroll_check(); \
        _print_raw("Running: ", 2, _cy, 0x07); \
        _print_raw(name, 11, _cy, 0x0F); \
        if (func()) { \
            _print_raw("[PASS]", 72, _cy, 0x0A); \
        } else { \
            _print_raw("[FAIL]", 72, _cy, 0x0C); \
        } \
        _cy++; \
    } while (0)

#endif
