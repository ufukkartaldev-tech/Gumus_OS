#include <stdint.h>

void gdt_flush(uint32_t ptr) { (void)ptr; }
void tss_flush(void) {}
