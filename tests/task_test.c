#include "framework.h"
#include "../src/kernel/cpu/task.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * @file task_test.c
 * @brief Multitasking ve Scheduler mantığını zorlar.
 */

// Dayı Tavsiyesi: Stack yapısını kontrol etmek için bir struct
struct task_stack_frame {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha
    uint32_t eip, cs, eflags;                       // iret
};

int test_scheduler_basics() {
    // 1. PID 0 ve Idle Task (Dayı Tavsiyesi 1)
    init_multitasking();
    
    ASSERT(current_task != NULL, "current_task must be initialized");
    ASSERT_EQ(current_task->id, 0, "Initial task must be PID 0 (Idle Task)");
    ASSERT_EQ(current_task->status, TASK_RUNNING, "Idle task must be RUNNING");
    
    // Dairesel liste kontrolü
    ASSERT(current_task->next == current_task, "Idle task should point to itself if alone");
    
    return TEST_PASS;
}

void _dummy_task_entry() {
    while(1) { __asm__ volatile("pause"); }
}

int test_task_stack_stitching() {
    // 2. Stack Dikiş Operasyonu (Dayı Tavsiyesi 2)
    create_task(_dummy_task_entry);
    
    task_t* new_task = current_task->next;
    ASSERT(new_task != current_task, "New task should be in the list");
    ASSERT_EQ(new_task->id, 1, "Next PID should be 1");
    
    // Stack Pointer (ESP) üzerinden sahte registerları kontrol et
    struct task_stack_frame* frame = (struct task_stack_frame*)new_task->esp;
    
    ASSERT_EQ(frame->eip, (uint32_t)_dummy_task_entry, "Task EIP must match entry point");
    ASSERT_EQ(frame->cs, 0x08, "Task CS must be Kernel Code (0x08)");
    ASSERT_EQ(frame->eflags, 0x202, "Task EFLAGS must have IF set (0x202)");
    
    return TEST_PASS;
}

int test_task_chaining_round_robin() {
    // 3. Ekmek Kuyruğu (Dayı Tavsiyesi 3)
    create_task(_dummy_task_entry); // PID 2
    
    task_t* task1 = current_task->next; // PID 1
    task_t* task2 = task1->next;       // PID 2
    
    ASSERT_EQ(task2->id, 2, "Task chaining order incorrect");
    ASSERT(task2->next == current_task, "List must be circular (PID 2 -> PID 0)");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("6. LAYER: SCHEDULER & TCB RIGOROUS");
    
    // Bellek ve Multitasking hazırla
    init_memory(16 * 1024 * 1024);
    
    RUN_TEST(test_scheduler_basics, "Idle Task (PID 0) Verification");
    RUN_TEST(test_task_stack_stitching, "Stack Stitching (Fake Frame)");
    RUN_TEST(test_task_chaining_round_robin, "Round Robin Circular List");
    
    _print_raw("Scheduler logic is consistent.", 2, 22, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
