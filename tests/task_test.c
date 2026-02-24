#include "framework.h"
#include "../src/kernel/cpu/task.h"
#include "../src/kernel/core/memory/memory.h"

/**
 * @file task_test.c
 * @brief Multitasking ve Scheduler mantığını test eder.
 */

int test_scheduler_basics() {
    // 1. İlklendirme
    init_multitasking();
    
    ASSERT(current_task != NULL, "current_task is globally accessible");
    ASSERT(current_task->id == 0, "Initial task (Kernel/Idle) must have PID 0");
    ASSERT(current_task->status == TASK_RUNNING, "Initial task must be in RUNNING state");
    
    return TEST_PASS;
}

void _dummy_task_entry() {
    while(1) { __asm__ volatile("pause"); }
}

int test_task_creation() {
    // 2. Yeni Task Oluşturma
    create_task(_dummy_task_entry);
    
    task_t* new_task = current_task->next;
    ASSERT(new_task != NULL, "New task was not added to the task list linked-list");
    ASSERT(new_task->id == 1, "New task should have PID 1");
    ASSERT(new_task->status == TASK_READY, "New task should start in READY state");
    
    // Stack Check: create_task stack'in sonuna başlangıç registerlarını koymalı
    ASSERT(new_task->esp != 0, "Task stack pointer (esp) was not initialized");
    
    // Bellek Sınırı: Task'ın kendi bellek alanı (Higher Half) kontrolü
    // GümüşOSHigher Half Kernel kullanıyorsa 0xC0000000 üzerinde olmalı
    // ASSERT((uint32_t)new_task->page_directory != 0, "Task must have its own page directory");

    return TEST_PASS;
}

void kernel_main() {
    test_header("SCHEDULER & TASK CONTROL BLOCK");
    
    init_memory(16 * 1024 * 1024);
    
    RUN_TEST(test_scheduler_basics, "Scheduler Initialization");
    RUN_TEST(test_task_creation, "Task Control Block (TCB) Setup");
    
    _print("Tasking tests completed.", 2, 12, 0x0A);
    while(1) { __asm__ volatile("hlt"); }
}
