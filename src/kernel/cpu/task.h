#ifndef TASK_H
#define TASK_H

#include <stdint.h>

// İşlem Kontrol Bloğu (Process Control Block - PCB)
typedef struct task_t {
    uint32_t esp;       // Stack Pointer (En önemli kayıt)
    uint32_t id;        // İşlem ID (PID)
    uint32_t status;    // 0: Bekliyor, 1: Çalışıyor, 2: Sonlandı
    uint32_t sleep_ticks; 
    void* page_directory; // Her sürecin kendi sayfa dizini
    uint32_t kernel_stack; // User modundan kernel moduna dönüş stack'i
    struct task_t* next;
} task_t;

// İşlem Yönetim Fonksiyonları
void init_multitasking();
void create_task(void (*entry_point)());
void create_user_process(void (*entry_point)());
void switch_task(uint32_t* current_esp); // Assembly tarafında çağrılacak

// Global: Şu an çalışan işlem
extern task_t* current_task;

#endif
