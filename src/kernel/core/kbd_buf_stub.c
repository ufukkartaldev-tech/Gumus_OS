#include <stdint.h>

static char kbdbuf[128];
static int kh = 0, kt = 0;

void kbd_put(char c) {
    int next = (kh + 1) % 128;
    if (next != kt) {
        kbdbuf[kh] = c;
        kh = next;
    }
}

char kbd_get() {
    if (kh == kt) return 0;
    char c = kbdbuf[kt];
    kt = (kt + 1) % 128;
    return c;
}
