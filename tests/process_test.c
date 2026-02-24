#include "test_framework.h"
#include "../src/kernel/cpu/task.h"
#include "../src/kernel/core/memory/memory.h"

static int test_var = 0;

void dummy_task() {
    test_var = 1;
    while(1) { __asm__ volatile("hlt"); }
}

int test_task_creation() {
    init_multitasking();
    
    // Check if current_task is set (PID 0)
    ASSERT(current_task != NULL, "Multitasking init failed to set current_task");
    ASSERT(current_task->id == 0, "Initial task should have PID 0");
    
    // Create a new task
    create_task(dummy_task);
    
    // Check if task was added to the list
    ASSERT(current_task->next != NULL, "Task was not added to the task list");
    ASSERT(current_task->next->id == 1, "New task should have PID 1");
    
    return 1;
}

int test_task_kill() {
    // Kill the task we just created
    int res = task_kill(1);
    ASSERT(res == 0, "task_kill failed");
    ASSERT(current_task->next->status == TASK_DEAD, "Task status should be DEAD");
    
    return 1;
}

void kernel_main() {
    TEST_HEADER("Process Management Tests");
    
    init_memory(16 * 1024 * 1024);
    
    RUN_TEST(test_task_creation, "Task Creation");
    RUN_TEST(test_task_kill, "Task Termination");
    
    TEST_FOOTER();
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
