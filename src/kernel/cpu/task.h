#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// GÃ¶rev DurumlarÄ±
#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEP   2
#define TASK_DEAD    3

// Syscall Ek
#define SYS_KILL     8
#define SYS_SHM_GET  9
#define SYS_SHM_AT   10

// Ä°ÅŸlem Kontrol BloÄŸu (Process Control Block - PCB)
typedef struct task_t {
    uint32_t esp;       // Stack Pointer (En Ã¶nemli kayÄ±t)
    uint32_t id;        // Ä°ÅŸlem ID (PID)
    uint32_t status;    // TASK_STATUS_*
    uint32_t sleep_ticks; 
    void* page_directory; // Her sÃ¼recin kendi sayfa dizini
    uint32_t kernel_stack; // User modundan kernel moduna dÃ¶nÃ¼ÅŸ stack'i
    uint32_t mem_break;    // SÃ¼reÃ§ bellek sÄ±nÄ±rÄ± (Heap iÃ§in)
    int exit_code;
    struct task_t* next;
} task_t;

// Ä°ÅŸlem YÃ¶netim FonksiyonlarÄ±
void init_multitasking();
void create_task(void (*entry_point)());
void create_user_process(void (*entry_point)());
void create_elf_task(uint32_t entry_point, void* page_directory, uint32_t mem_break);
void task_exit(int code);
int task_kill(uint32_t pid);
void switch_task(uint32_t* current_esp); // Assembly tarafÄ±nda Ã§aÄŸrÄ±lacak

// Global: Åu an Ã§alÄ±ÅŸan iÅŸlem
extern task_t* current_task;
uint32_t schedule(uint32_t esp);

#endif
