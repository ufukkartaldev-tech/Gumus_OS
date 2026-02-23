#include "idt.h"

// Assembly function implementations
void isr0() {
    // Division by zero handler
    while(1);
}

void irq0() {
    // Timer interrupt
    while(1);
}

void irq1() {
    // Keyboard interrupt
    while(1);
}

void irq12() {
    // Mouse interrupt
    while(1);
}

void isr128() {
    // System call interrupt
    while(1);
}

// GDT functions
void gdt_flush(uint32_t gdt_ptr) {
    // GDT flush implementation
    (void)gdt_ptr;
}

void tss_flush() {
    // TSS flush implementation
}
