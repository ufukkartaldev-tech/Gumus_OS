// GümüşOS Process Management Unit Test
// Process yönetimi fonksiyonlarını test eder

#include <stddef.h>
#include <stdint.h>

// Process yönetimi sabitleri
#define MAX_PROCESSES 64
#define KERNEL_STACK_SIZE 4096
#define USER_STACK_SIZE 8192

// Process durumları
typedef enum {
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_TERMINATED
} process_state_t;

// Process öncelikleri
typedef enum {
    PRIORITY_IDLE = 0,
    PRIORITY_LOW = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_HIGH = 3,
    PRIORITY_REALTIME = 4
} process_priority_t;

// Process yapısı
typedef struct {
    uint32_t pid;
    process_state_t state;
    process_priority_t priority;
    uint32_t* stack_pointer;
    uint32_t* instruction_pointer;
    uint32_t kernel_stack[KERNEL_STACK_SIZE / 4];
    uint32_t user_stack[USER_STACK_SIZE / 4];
    uint32_t cpu_time;
    uint32_t wait_time;
} process_t;

// Process tablosu
static process_t process_table[MAX_PROCESSES];
static uint32_t current_pid = 0;
static uint32_t running_pid = 0;

// Test sonuçları
typedef enum {
    TEST_PASS,
    TEST_FAIL
} test_result_t;

// Test mesajları
static const char* MSG_START = "Process Management Test Suite Baslatildi...";
static const char* MSG_CREATE = "Process Creation Testi: ";
static const char* MSG_SCHEDULE = "Scheduler Testi: ";
static const char* MSG_SWITCH = "Context Switch Testi: ";
static const char* MSG_TERMINATE = "Process Termination Testi: ";
static const char* MSG_PASS = "PASS";
static const char* MSG_FAIL = "FAIL";
static const char* MSG_DONE = "Process Management Testleri Tamamlandi!";

// VGA yardımcı fonksiyonları
static void vga_putchar(char c, int x, int y, char color) {
    char* vga = (char*)0xB8000;
    vga[y * 80 * 2 + x * 2] = c;
    vga[y * 80 * 2 + x * 2 + 1] = color;
}

static void vga_print(const char* str, int x, int y, char color) {
    for (int i = 0; str[i]; i++) {
        vga_putchar(str[i], x + i, y, color);
    }
}

static void vga_clear_screen() {
    char* vga = (char*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x0F;
    }
}

// Process yönetimi fonksiyonları
static uint32_t create_process(process_priority_t priority, void (*entry_point)(void)) {
    if (current_pid >= MAX_PROCESSES) {
        return 0; // Hata
    }
    
    process_t* proc = &process_table[current_pid];
    proc->pid = current_pid + 1;
    proc->state = PROCESS_STATE_READY;
    proc->priority = priority;
    proc->cpu_time = 0;
    proc->wait_time = 0;
    
    // Stack pointer'ı ayarla
    proc->stack_pointer = (uint32_t*)&proc->kernel_stack[KERNEL_STACK_SIZE / 4 - 1];
    proc->instruction_pointer = (uint32_t*)entry_point;
    
    current_pid++;
    return proc->pid;
}

static void terminate_process(uint32_t pid) {
    if (pid == 0 || pid > MAX_PROCESSES) {
        return;
    }
    
    process_t* proc = &process_table[pid - 1];
    proc->state = PROCESS_STATE_TERMINATED;
    
    // Eğer çalışan process ise bir sonraki ready process'e geç
    if (pid == running_pid) {
        running_pid = 0;
    }
}

static uint32_t schedule_next_process() {
    uint32_t highest_priority = 0;
    uint32_t next_pid = 0;
    
    for (uint32_t i = 0; i < current_pid; i++) {
        process_t* proc = &process_table[i];
        
        if (proc->state == PROCESS_STATE_READY && 
            proc->priority >= highest_priority) {
            highest_priority = proc->priority;
            next_pid = proc->pid;
        }
    }
    
    return next_pid;
}

static void context_switch(uint32_t from_pid, uint32_t to_pid) {
    if (from_pid != 0 && from_pid <= MAX_PROCESSES) {
        process_t* from_proc = &process_table[from_pid - 1];
        from_proc->state = PROCESS_STATE_READY;
    }
    
    if (to_pid != 0 && to_pid <= MAX_PROCESSES) {
        process_t* to_proc = &process_table[to_pid - 1];
        to_proc->state = PROCESS_STATE_RUNNING;
        running_pid = to_pid;
    }
}

// Test process fonksiyonları
static void test_process_1(void) {
    // Test process 1
    volatile int counter = 0;
    for (int i = 0; i < 1000; i++) {
        counter++;
    }
}

static void test_process_2(void) {
    // Test process 2
    volatile int counter = 0;
    for (int i = 0; i < 2000; i++) {
        counter++;
    }
}

static void test_process_3(void) {
    // Test process 3
    volatile int counter = 0;
    for (int i = 0; i < 500; i++) {
        counter++;
    }
}

// Test fonksiyonları
static test_result_t test_process_creation() {
    vga_print(MSG_CREATE, 0, 0, 0x0F);
    
    // Process tablosunu temizle
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_STATE_TERMINATED;
    }
    current_pid = 0;
    
    // Process'ler oluştur
    uint32_t pid1 = create_process(PRIORITY_NORMAL, test_process_1);
    uint32_t pid2 = create_process(PRIORITY_HIGH, test_process_2);
    uint32_t pid3 = create_process(PRIORITY_LOW, test_process_3);
    
    // PID'lerin doğru olduğunu kontrol et
    if (pid1 != 1 || pid2 != 2 || pid3 != 3) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Process durumlarını kontrol et
    if (process_table[0].state != PROCESS_STATE_READY ||
        process_table[1].state != PROCESS_STATE_READY ||
        process_table[2].state != PROCESS_STATE_READY) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Öncelikleri kontrol et
    if (process_table[0].priority != PRIORITY_NORMAL ||
        process_table[1].priority != PRIORITY_HIGH ||
        process_table[2].priority != PRIORITY_LOW) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 0, 0x0A);
    return TEST_PASS;
}

static test_result_t test_scheduler() {
    vga_print(MSG_SCHEDULE, 0, 1, 0x0F);
    
    // Scheduler testi - en yüksek öncelikli process seçilmeli
    uint32_t next_pid = schedule_next_process();
    
    if (next_pid != 2) { // PRIORITY_HIGH olan process
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    // Process 2'yi terminate et
    terminate_process(2);
    
    // Şimdi PRIORITY_NORMAL olan process seçilmeli
    next_pid = schedule_next_process();
    
    if (next_pid != 1) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 1, 0x0A);
    return TEST_PASS;
}

static test_result_t test_context_switch() {
    vga_print(MSG_SWITCH, 0, 2, 0x0F);
    
    // Context switch testi
    uint32_t from_pid = running_pid;
    uint32_t to_pid = 1;
    
    context_switch(from_pid, to_pid);
    
    // Running PID'nin değiştiğini kontrol et
    if (running_pid != 1) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // Process durumunu kontrol et
    if (process_table[0].state != PROCESS_STATE_RUNNING) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 2, 0x0A);
    return TEST_PASS;
}

static test_result_t test_process_termination() {
    vga_print(MSG_TERMINATE, 0, 3, 0x0F);
    
    // Process termination testi
    terminate_process(1);
    terminate_process(3);
    
    // Process durumlarını kontrol et
    if (process_table[0].state != PROCESS_STATE_TERMINATED ||
        process_table[2].state != PROCESS_STATE_TERMINATED) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Running PID'nin sıfırlandığını kontrol et
    if (running_pid != 0) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Scheduler'ın hiç process bulamaması gerekiyor
    uint32_t next_pid = schedule_next_process();
    if (next_pid != 0) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 3, 0x0A);
    return TEST_PASS;
}

// Ana test fonksiyonu
void kernel_main() {
    vga_clear_screen();
    vga_print(MSG_START, 0, 0, 0x0F);
    
    // Process creation testleri
    test_result_t result = test_process_creation();
    
    // Scheduler testleri
    result = test_scheduler();
    
    // Context switch testleri
    result = test_context_switch();
    
    // Process termination testleri
    result = test_process_termination();
    
    // Test sonu
    vga_print(MSG_DONE, 0, 5, 0x0F);
    
    // Sonsuz döngü
    while (1) {
        __asm__ volatile("hlt");
    }
}
