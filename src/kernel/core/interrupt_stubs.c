#include "idt.h"

// Strong definitions to provide missing symbols
void isr0() { while(1); }
void irq0() { while(1); }
void irq1() { while(1); }
void irq12() { while(1); }
void isr128() { while(1); }
void gdt_flush(uint32_t gdt_ptr) { (void)gdt_ptr; }
void tss_flush() { }
void isr_handler(registers_t* regs) { (void)regs; }
void irq_handler(registers_t* regs) { (void)regs; }
uint32_t syscall_handler(registers_t* regs) { (void)regs; return 0; }
void handle_keyboard(uint8_t scancode) { (void)scancode; }
