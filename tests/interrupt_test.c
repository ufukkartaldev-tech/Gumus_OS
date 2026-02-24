#include "framework.h"

/**
 * 5. KATMAN: INTERRUPTS & EXCEPTIONS (GÜVENLİK TESTİ)
 */

int test_divide_by_zero() {
    /**
     * DİKKAT: Bilerek 0'a bölme işlemi.
     * IDT düzgün yüklendiyse sistem 'Divide by Zero' mesajı verip durmalı.
     */
    _print_raw("[WARN] Divide-by-zero test will STOP the CPU!", 2, _cy++, 0x0E);
    
    // Testi aktifleştirmek için aşağıdaki yorumu kaldırabilirsin:
    /*
    volatile int a = 10;
    volatile int b = 0;
    volatile int c = a / b; (void)c;
    */
    
    return TEST_PASS;
}

int test_soft_interrupt() {
    /**
     * Yazılımsal Kesme (Software Interrupt)
     * int 0x80 (GümüşOS Syscall) sistem kilitlenmeden dönüyor mu?
     */
    _print_raw("Triggering INT 0x80 (Syscall)... ", 2, _cy, 0x07);
    __asm__ volatile("int $0x80");
    _print_raw("DONE", 40, _cy++, 0x0A);
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("5. LAYER: INTERRUPTS & SECURITY");
    
    // GDT/IDT init edilmeli ki testler çalışsın (Mock değil gerçek lazım)
    // init_gdt();
    // init_idt();
    
    RUN_TEST(test_soft_interrupt, "Software Interrupt (INT 0x80)");
    RUN_TEST(test_divide_by_zero, "Exception Handling (Div/0)");
    
    _print_raw("Interrupt tests done.", 2, 20, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
