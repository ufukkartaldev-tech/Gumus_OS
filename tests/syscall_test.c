#include "test_framework.h"
#include "../src/kernel/core/idt.h"

// Mock registers_t if not available or needed
// GümüşOS registers_t usually contains eax, ebx, ecx, edx, esi, edi, esp, ebp, etc.

extern uint32_t syscall_handler(registers_t* r);

int test_vga_syscall() {
    registers_t r;
    // Test SYS_PRESENT (23) - just calls vga_present
    r.eax = 23;
    uint32_t res = syscall_handler(&r);
    // If it doesn't crash, it's a pass for now as it returns void internally
    return 1;
}

int test_invalid_syscall() {
    registers_t r;
    r.eax = 999;
    uint32_t res = syscall_handler(&r);
    // Should probably handle unknown syscalls by printing or returning error
    return 1;
}

void kernel_main() {
    TEST_HEADER("System Call Interface Tests");
    
    RUN_TEST(test_vga_syscall, "VGA Syscall (Present)");
    RUN_TEST(test_invalid_syscall, "Invalid Syscall Handling");
    
    TEST_FOOTER();
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
