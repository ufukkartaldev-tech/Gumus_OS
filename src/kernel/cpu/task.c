#include "task.h"
#include "memory.h"
#include "kernel.h"
#include "gdt.h"
#include "string.h"

// GÃ¶rev Listesi (Linked List)
task_t* task_head = 0;
task_t* current_task = 0;
static uint32_t next_pid = 1;

// Ä°ÅŸlemci Register YapÄ±sÄ± (Stack Ã¼zerinde saklanacak)
// pusha -> edi, esi, ebp, esp, ebx, edx, ecx, eax
// iret -> eip, cs, eflags
struct task_stack {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pusha
    uint32_t eip, cs, eflags; // iret tarafÄ±ndan kullanÄ±lÄ±r
};

void init_multitasking() {
    // Kernel'Ä±n kendisi de bir gÃ¶revdir (PID 0)
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->id = 0;
    kernel_task->status = 1; // Ã‡alÄ±ÅŸÄ±yor
    kernel_task->sleep_ticks = 0;
    kernel_task->page_directory = (void*)kernel_directory;
    kernel_task->kernel_stack = 0; // Kernel zaten kernel modunda
    kernel_task->next = kernel_task; // Kendine dÃ¶nÃ¼yor (Dairesel Liste BaÅŸlangÄ±cÄ±)
    
    task_head = kernel_task;
    current_task = kernel_task;
    next_pid = 1;
}

void create_task(void (*entry_point)()) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    new_task->id = next_pid++;
    new_task->status = 0;
    new_task->sleep_ticks = 0;
    new_task->page_directory = (void*)kernel_directory;
    new_task->kernel_stack = 0;

    uint32_t* stack = (uint32_t*)kmalloc(4096);
    uint32_t* top = stack + 1024;

    top--; *top = 0x202; 
    top--; *top = 0x08;  
    top--; *top = (uint32_t)entry_point; 

    top--; *top = 0; // EAX
    top--; *top = 0; // ECX
    top--; *top = 0; // EDX
    top--; *top = 0; // EBX
    top--; *top = 0; // ESP
    top--; *top = 0; // EBP
    top--; *top = 0; // ESI
    top--; *top = 0; // EDI

    new_task->esp = (uint32_t)top;

    if (task_head == 0) {
        task_head = new_task;
        new_task->next = new_task;
        current_task = new_task;
    } else {
        task_t* temp = task_head;
        while(temp->next != task_head) temp = temp->next;
        temp->next = new_task;
        new_task->next = task_head;
    }
}

void create_user_process(void (*entry_point)()) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    new_task->id = next_pid++;
    new_task->status = 0;
    new_task->sleep_ticks = 0;
    
    // Her sÃ¼recin kendi sayfa dizini
    new_task->page_directory = create_process_directory();
    
    // Kernel Stack (User modundan kesme gelince buraya dÃ¶nÃ¼lÃ¼r)
    uint32_t* kstack = (uint32_t*)kmalloc(4096);
    new_task->kernel_stack = (uint32_t)kstack + 4096;

    // User Stack (KullanÄ±cÄ± modu yÄ±ÄŸÄ±nÄ±)
    // GerÃ§ek bir OS'te bu sayfa dizininde user-space'e maplenir (Ã¶rn: 0xBFFFF000)
    // Åimdilik basitleÅŸtirmek iÃ§in PMM'den bir sayfa alÄ±p mapleyelim.
    void* user_stack_phys = pmm_alloc_frame();
    uint32_t user_stack_virt = 0x80000000; // 2GB sÄ±nÄ±rÄ±
    map_page_in_dir(new_task->page_directory, user_stack_phys, (void*)user_stack_virt, 0x7); // User, RW, Present

    uint32_t* top = (uint32_t*)(new_task->kernel_stack);

    // IRET stack'i (User Mode'a geÃ§iÅŸ iÃ§in)
    top--; *top = 0x23; // SS (User Data + RPL 3)
    top--; *top = user_stack_virt + 4096; // ESP (User stack top)
    top--; *top = 0x202; // EFLAGS
    top--; *top = 0x1B; // CS (User Code + RPL 3)
    top--; *top = (uint32_t)entry_point;

    // PUSHA
    for(int i = 0; i < 8; i++) { top--; *top = 0; }

    new_task->esp = (uint32_t)top;

    // Listeye Ekle
    task_t* temp = task_head;
    while(temp->next != task_head) temp = temp->next;
    temp->next = new_task;
    new_task->next = task_head;
}

void create_elf_task(uint32_t entry_point, void* page_directory, uint32_t mem_break) {
    task_t* new_task = (task_t*)kmalloc(sizeof(task_t));
    new_task->id = next_pid++;
    new_task->status = 0;
    new_task->sleep_ticks = 0;
    new_task->page_directory = page_directory;
    new_task->mem_break = mem_break;
    
    // Kernel Stack
    uint32_t* kstack = (uint32_t*)kmalloc(4096);
    new_task->kernel_stack = (uint32_t)kstack + 4096;

    // User Stack (KullanÄ±cÄ± YÄ±ÄŸÄ±nÄ±)
    // 0xBFFFF000 adresine (Kernel sÄ±nÄ±rÄ±nÄ±n hemen altÄ±na) mapliyoruz
    void* user_stack_phys = pmm_alloc_frame();
    uint32_t user_stack_virt = 0xBFFFF000; 
    map_page_in_dir(new_task->page_directory, user_stack_phys, (void*)user_stack_virt, 0x7);

    uint32_t* top = (uint32_t*)(new_task->kernel_stack);

    // IRET stack
    top--; *top = 0x23; // SS
    top--; *top = user_stack_virt + 4096; // ESP (Stack top)
    top--; *top = 0x202; // EFLAGS
    top--; *top = 0x1B; // CS
    top--; *top = entry_point;

    // PUSHA
    for(int i = 0; i < 8; i++) { top--; *top = 0; }

    new_task->esp = (uint32_t)top;

    // Listeye Ekle
    task_t* temp = task_head;
    while(temp->next != task_head) temp = temp->next;
    temp->next = new_task;
    new_task->next = task_head;
}

void task_exit(int code) {
    if (current_task) {
        current_task->status = TASK_DEAD;
        current_task->exit_code = code;
        
        // Bu noktadan sonra scheduler bir sonraki tick'te bu gÃ¶revi temizleyecek.
        // Hemen gÃ¶rev deÄŸiÅŸimi yapmak iÃ§in timer kesmesini tetiklemek yerine 
        // syscall handler'Ä±n gÃ¶rev deÄŸiÅŸtirmesine gÃ¼veneceÄŸiz.
        print_color("\n[GumusOS] Islem sonlandi. (PID: ", YELLOW);
        char buf[16];
        itoa(current_task->id, buf); print_color(buf, YELLOW);
        print_color(", Kod: ", YELLOW);
        itoa(code, buf); print_color(buf, YELLOW);
        print_color(")\n", YELLOW);
    }
}

int task_kill(uint32_t pid) {
    if (pid == 0) return -1; // Kernel'Ä± Ã¶ldÃ¼rme
    
    task_t* temp = task_head;
    do {
        if (temp->id == pid) {
            temp->status = TASK_DEAD;
            temp->exit_code = -1; // KapatÄ±ldÄ±
            return 0;
        }
        temp = temp->next;
    } while (temp != task_head);
    
    return -1; // BulunamadÄ±
}

uint32_t schedule(uint32_t esp) {
    if (!current_task) return esp;

    // Åu anki gÃ¶revin stack'ini sakla
    current_task->esp = esp;

    // 1. Durum: EÄŸer Mevcut GÃ¶rev Ã–ldÃ¼yse Onu Listeden Ã‡Ä±kar (Cleanup)
    // Not: PID 0 (Kernel) asla Ã¶lmez, bu yÃ¼zden dairesel liste asla boÅŸ kalmaz.
    task_t* prev = task_head;
    while (prev->next != current_task) prev = prev->next;

    if (current_task->status == TASK_DEAD && current_task->id != 0) {
        task_t* to_delete = current_task;
        prev->next = current_task->next;
        current_task = current_task->next; // Bir sonrakine geÃ§
        
        // BelleÄŸi kÄ±smen temizle (Ä°leride daha kapsamlÄ± yapÄ±lacak)
        // kfree(to_delete->page_directory); 
        // kfree(page_of_kernel_stack);
        kfree(to_delete);
    } else {
        // Normal geÃ§iÅŸ: Bir sonraki gÃ¶reve atla
        current_task = current_task->next;
    }

    // 2. Durum: EÄŸer yeni seÃ§ilen gÃ¶rev de hazÄ±r deÄŸilse, hazÄ±r olanÄ± bulana kadar dÃ¶n
    while (current_task->status == TASK_DEAD || current_task->status == TASK_SLEEP) {
        current_task = current_task->next;
    }

    // Sayfa dizinini deÄŸiÅŸtir (Bellek Ä°zolasyonu)
    if (current_task->page_directory) {
        switch_page_directory((page_directory_t*)current_task->page_directory);
    }

    // TSS Kernel Stack'i gÃ¼ncelle (Ring 3 -> Ring 0 geÃ§iÅŸi iÃ§in)
    if (current_task->kernel_stack) {
        tss_set_stack(0x10, current_task->kernel_stack);
    }

    return current_task->esp;
}
