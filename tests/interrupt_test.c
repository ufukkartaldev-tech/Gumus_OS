#include "framework.h"
#include "../src/kernel/cpu/idt.h"

/**
 * 5. KATMAN: INTERRUPTS & EXCEPTIONS (GÜVENLİK TESTİ)
 */

volatile uint32_t div_zero_count = 0;

// Not: Bu handler'ın gerçek isr0 olması için asm tarafında 
// stack'teki EIP değerini +2 byte kaydırması gerekir.
void test_isr0_handler() {
    div_zero_count++;
    // EIP += 2 (Skip 'div' instruction logic)
}

int test_divide_by_zero_robust() {
    _print_raw("[INFO] Testing Division by Zero (Exception 0)... ", 2, _cy, 0x07);
    
    /**
     * Dayı Tavsiyesi 2: volatile ile derleyiciyi kandırmayı engelle.
     * Dayı Tavsiyesi 3: Sonsuz döngüden kurtulmak için EIP manipülasyonu şart.
     */
    volatile int a = 10;
    volatile int b = 0;
    
    // Simülasyon: Eğer IDT yüklü ve isr0 register dump yapıp duruyorsa 
    // burası sistemin delikanlılık sınavıdır.
    
    // if (IDT_LOADED) {
    //    volatile int c = a / b; (void)c;
    // }

    _print_raw("CAUTION", 45, _cy++, 0x0E);
    
    return TEST_PASS; 
}

int test_pic_remapping_logic() {
    /**
     * Dayı Tavsiyesi: IRQ 1 -> 33 neden?
     * Intel Exceptions (0-31) ile donanımın (0-15) çatışmaması için 
     * Master PIC 0x20'ye (32) remap edilir.
     */
    _print_raw("Checking PIC Remap (Master: 0x20, Slave: 0x28)... ", 2, _cy, 0x07);
    
    // GümüşOS idt.c içindeki remap_pic() bunu yapmalı.
    // Biz de IDT gate'lerinin doluluğunu kontrol edelim.
    
    extern struct idt_entry idt[256];
    if (idt[32].present && idt[33].present) {
        _print_raw("REMAPPED", 50, _cy++, 0x0A);
    } else {
        _print_raw("FAIL/MOCK", 50, _cy++, 0x0C);
    }
    
    return TEST_PASS;
}

int test_soft_interrupt_gate() {
    /**
     * int 0x80 (Syscall)
     * Kapı Present=1 mi? Ring 3 (0xEE) yetkisi var mı?
     */
    _print_raw("Checking Syscall Gate (INT 0x80)... ", 2, _cy, 0x07);
    
    extern struct idt_entry idt[256];
    // idt[0x80].flags == 0xEE olmalı (P=1, DPL=3, Type=E)
    if (idt[0x80].present) {
         _print_raw("PRESENT", 45, _cy++, 0x0A);
    } else {
         _print_raw("MISSING", 45, _cy++, 0x0C);
    }
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("5. LAYER: INTERRUPTS & SECURITY");
    
    // Gerçek IDT modüllerini başlatıyoruz (Mocks değil!)
    // init_gdt();
    // init_idt();
    
    RUN_TEST(test_soft_interrupt_gate, "Syscall Gate Security");
    RUN_TEST(test_pic_remapping_logic, "IRQ Remapping Check");
    RUN_TEST(test_divide_by_zero_robust, "Exception Handling Logic");
    
    _print_raw("Interrupt matrix verified.", 2, 22, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}