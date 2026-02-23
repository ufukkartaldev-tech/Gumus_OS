#include <stdint.h>

// Simple abs implementation
int abs(int x) {
    return x < 0 ? -x : x;
}

// Simple stack check function
void __chkstk_ms() {
    // Stack checking - no-op for now
}
