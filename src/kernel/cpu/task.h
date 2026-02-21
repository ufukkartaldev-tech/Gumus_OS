#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// Görev Durumları
#define TASK_READY   0
#define TASK_RUNNING 1
#define TASK_SLEEP   2
#define TASK_DEAD    3

// İşlem Kontrol Bloğu (Process Control Block - PCB)
typedef struct task_t {
    uint32_t esp;       // Stack Pointer (En önemli kayıt)
    uint32_t id;        // İşlem ID (PID)
    uint32_t status;    // TASK_STATUS_*
    uint32_t sleep_ticks; 
    void* page_directory; // Her sürecin kendi sayfa dizini
    uint32_t kernel_stack; // User modundan kernel moduna dönüş stack'i
    uint32_t mem_break;    // Süreç bellek sınırı (Heap için)
    int exit_code;
    struct task_t* next;
} task_t;

// İşlem Yönetim Fonksiyonları
void init_multitasking();
void create_task(void (*entry_point)());
void create_user_process(void (*entry_point)());
void create_elf_task(uint32_t entry_point, void* page_directory, uint32_t mem_break);
void task_exit(int code);
void switch_task(uint32_t* current_esp); // Assembly tarafında çağrılacak

// Global: Şu an çalışan işlem
extern task_t* current_task;

#endif
