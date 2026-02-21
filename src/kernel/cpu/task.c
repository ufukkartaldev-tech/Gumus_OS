#include "task.h"
#include "memory.h"
#include "kernel.h"
#include "gdt.h"

// Görev Listesi (Linked List)
task_t* task_head = 0;
task_t* current_task = 0;
static uint32_t next_pid = 1;

// İşlemci Register Yapısı (Stack üzerinde saklanacak)
// pusha -> edi, esi, ebp, esp, ebx, edx, ecx, eax
// iret -> eip, cs, eflags
struct task_stack {
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pusha
    uint32_t eip, cs, eflags; // iret tarafından kullanılır
};

void init_multitasking() {
    // Kernel'ın kendisi de bir görevdir (PID 0)
    task_t* kernel_task = (task_t*)kmalloc(sizeof(task_t));
    kernel_task->id = 0;
    kernel_task->status = 1; // Çalışıyor
    kernel_task->sleep_ticks = 0;
    kernel_task->page_directory = (void*)kernel_directory;
    kernel_task->kernel_stack = 0; // Kernel zaten kernel modunda
    kernel_task->next = kernel_task; // Kendine dönüyor (Dairesel Liste Başlangıcı)
    
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
    
    // Her sürecin kendi sayfa dizini
    new_task->page_directory = create_process_directory();
    
    // Kernel Stack (User modundan kesme gelince buraya dönülür)
    uint32_t* kstack = (uint32_t*)kmalloc(4096);
    new_task->kernel_stack = (uint32_t)kstack + 4096;

    // User Stack (Kullanıcı modu yığını)
    // Gerçek bir OS'te bu sayfa dizininde user-space'e maplenir (örn: 0xBFFFF000)
    // Şimdilik basitleştirmek için PMM'den bir sayfa alıp mapleyelim.
    void* user_stack_phys = pmm_alloc_frame();
    uint32_t user_stack_virt = 0x80000000; // 2GB sınırı
    map_page_in_dir(new_task->page_directory, user_stack_phys, (void*)user_stack_virt, 0x7); // User, RW, Present

    uint32_t* top = (uint32_t*)(new_task->kernel_stack);

    // IRET stack'i (User Mode'a geçiş için)
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

    // User Stack (Kullanıcı Yığını)
    // 0xBFFFF000 adresine (Kernel sınırının hemen altına) mapliyoruz
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

uint32_t schedule(uint32_t esp) {
    if (!current_task) return esp;

    current_task->esp = esp;
    current_task = current_task->next;

    // Sayfa dizinini değiştir (Bellek İzolasyonu)
    if (current_task->page_directory) {
        switch_page_directory((page_directory_t*)current_task->page_directory);
    }

    // TSS Kernel Stack'i güncelle (Ring 3 -> Ring 0 geçişi için)
    if (current_task->kernel_stack) {
        tss_set_stack(0x10, current_task->kernel_stack);
    }

    return current_task->esp;
}
