// GümüşOS Syscall Interface Unit Test
// System call interface'ini test eder

#include <stddef.h>
#include <stdint.h>

// Syscall numaraları
#define SYS_EXIT    1
#define SYS_WRITE   2
#define SYS_READ    3
#define SYS_OPEN    4
#define SYS_CLOSE   5
#define SYS_MALLOC  6
#define SYS_FREE    7
#define SYS_GETPID  8
#define SYS_FORK    9

// Syscall sonuçları
typedef enum {
    SYS_SUCCESS = 0,
    SYS_ERROR = -1
} syscall_result_t;

// Process yapısı
typedef struct {
    uint32_t pid;
    uint32_t parent_pid;
    uint32_t uid;
    uint32_t gid;
} process_info_t;

// Dosya yapısı
typedef struct {
    uint32_t fd;
    uint32_t flags;
    uint32_t mode;
    uint32_t offset;
} file_info_t;

// Test sonuçları
typedef enum {
    TEST_PASS,
    TEST_FAIL
} test_result_t;

// Test mesajları
static const char* MSG_START = "Syscall Interface Test Suite Baslatildi...";
static const char* MSG_BASIC = "Basic Syscall Testi: ";
static const char* MSG_WRITE = "Write Syscall Testi: ";
static const char* MSG_MALLOC = "Memory Syscall Testi: ";
static const char* MSG_PROCESS = "Process Syscall Testi: ";
static const char* MSG_SECURITY = "Security Testi: ";
static const char* MSG_PASS = "PASS";
static const char* MSG_FAIL = "FAIL";
static const char* MSG_DONE = "Syscall Interface Testleri Tamamlandi!";

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

// Syscall implementasyonları
static syscall_result_t sys_exit(int exit_code) {
    // Process'i sonlandır
    (void)exit_code;
    return SYS_SUCCESS;
}

static syscall_result_t sys_write(uint32_t fd, const void* buffer, size_t count) {
    if (fd != 1 && fd != 2) { // Sadece stdout ve stderr
        return SYS_ERROR;
    }
    
    if (!buffer || count == 0) {
        return SYS_ERROR;
    }
    
    // VGA'ya yaz
    const char* str = (const char*)buffer;
    char* vga = (char*)0xB8000;
    
    for (size_t i = 0; i < count && str[i]; i++) {
        vga[i * 2] = str[i];
        vga[i * 2 + 1] = 0x0F;
    }
    
    return SYS_SUCCESS;
}

static syscall_result_t sys_read(uint32_t fd, void* buffer, size_t count) {
    if (fd != 0) { // Sadece stdin
        return SYS_ERROR;
    }
    
    if (!buffer || count == 0) {
        return SYS_ERROR;
    }
    
    // Simüle edilmiş okuma
    char* buf = (char*)buffer;
    buf[0] = 'T';
    buf[1] = 'E';
    buf[2] = 'S';
    buf[3] = 'T';
    
    return SYS_SUCCESS;
}

static void* sys_malloc(size_t size) {
    if (size == 0 || size > 0x10000) { // 64KB limit
        return NULL;
    }
    
    // Basit heap implementasyonu
    static uint8_t heap[0x10000];
    static size_t heap_used = 0;
    
    if (heap_used + size > sizeof(heap)) {
        return NULL;
    }
    
    void* ptr = &heap[heap_used];
    heap_used += size;
    return ptr;
}

static syscall_result_t sys_free(void* ptr) {
    // Basit implementasyon - gerçek OS'ta daha karmaşık
    (void)ptr;
    return SYS_SUCCESS;
}

static uint32_t sys_getpid() {
    return 1234; // Simüle edilmiş PID
}

static uint32_t sys_fork() {
    return 5678; // Simüle edilmiş child PID
}

// Syscall dispatcher
static uint32_t syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    switch (syscall_num) {
        case SYS_EXIT:
            return sys_exit(arg1);
        case SYS_WRITE:
            return sys_write(arg1, (const void*)arg2, arg3);
        case SYS_READ:
            return sys_read(arg1, (void*)arg2, arg3);
        case SYS_MALLOC:
            return (uint32_t)sys_malloc(arg1);
        case SYS_FREE:
            return sys_free((void*)arg1);
        case SYS_GETPID:
            return sys_getpid();
        case SYS_FORK:
            return sys_fork();
        default:
            return SYS_ERROR;
    }
}

// Test fonksiyonları
static test_result_t test_basic_syscalls() {
    vga_print(MSG_BASIC, 0, 0, 0x0F);
    
    // Geçersiz syscall testi
    uint32_t result = syscall_handler(999, 0, 0, 0);
    if (result != SYS_ERROR) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    // Exit syscall testi
    result = syscall_handler(SYS_EXIT, 0, 0, 0);
    if (result != SYS_SUCCESS) {
        vga_print(MSG_FAIL, 50, 0, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 0, 0x0A);
    return TEST_PASS;
}

static test_result_t test_write_syscall() {
    vga_print(MSG_WRITE, 0, 1, 0x0F);
    
    // Write syscall testi
    const char* test_msg = "TEST";
    uint32_t result = syscall_handler(SYS_WRITE, 1, (uint32_t)test_msg, 4);
    
    if (result != SYS_SUCCESS) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    // VGA'da kontrol et
    char* vga = (char*)0xB8000;
    if (vga[0] != 'T' || vga[2] != 'E' || vga[4] != 'S' || vga[6] != 'T') {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    // Geçersiz fd testi
    result = syscall_handler(SYS_WRITE, 99, (uint32_t)test_msg, 4);
    if (result != SYS_ERROR) {
        vga_print(MSG_FAIL, 50, 1, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 1, 0x0A);
    return TEST_PASS;
}

static test_result_t test_memory_syscalls() {
    vga_print(MSG_MALLOC, 0, 2, 0x0F);
    
    // Malloc syscall testi
    void* ptr1 = (void*)syscall_handler(SYS_MALLOC, 100, 0, 0);
    void* ptr2 = (void*)syscall_handler(SYS_MALLOC, 200, 0, 0);
    
    if (!ptr1 || !ptr2) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // Pointer'ların farklı olduğunu kontrol et
    if (ptr1 == ptr2) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // Free syscall testi
    uint32_t result = syscall_handler(SYS_FREE, (uint32_t)ptr1, 0, 0);
    if (result != SYS_SUCCESS) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    // Geçersiz malloc testi
    void* ptr3 = (void*)syscall_handler(SYS_MALLOC, 0, 0, 0);
    if (ptr3 != NULL) {
        vga_print(MSG_FAIL, 50, 2, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 2, 0x0A);
    return TEST_PASS;
}

static test_result_t test_process_syscalls() {
    vga_print(MSG_PROCESS, 0, 3, 0x0F);
    
    // Getpid syscall testi
    uint32_t pid = syscall_handler(SYS_GETPID, 0, 0, 0);
    if (pid != 1234) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Fork syscall testi
    uint32_t child_pid = syscall_handler(SYS_FORK, 0, 0, 0);
    if (child_pid != 5678) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Read syscall testi
    char buffer[10];
    uint32_t result = syscall_handler(SYS_READ, 0, (uint32_t)buffer, 10);
    if (result != SYS_SUCCESS) {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    // Buffer kontrolü
    if (buffer[0] != 'T' || buffer[1] != 'E' || buffer[2] != 'S' || buffer[3] != 'T') {
        vga_print(MSG_FAIL, 50, 3, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 3, 0x0A);
    return TEST_PASS;
}

static test_result_t test_security() {
    vga_print(MSG_SECURITY, 0, 4, 0x0F);
    
    // Security testi - NULL pointer kontrolü
    uint32_t result = syscall_handler(SYS_WRITE, 1, 0, 10);
    if (result != SYS_ERROR) {
        vga_print(MSG_FAIL, 50, 4, 0x0C);
        return TEST_FAIL;
    }
    
    // Sıfır length kontrolü
    const char* test_msg = "TEST";
    result = syscall_handler(SYS_WRITE, 1, (uint32_t)test_msg, 0);
    if (result != SYS_ERROR) {
        vga_print(MSG_FAIL, 50, 4, 0x0C);
        return TEST_FAIL;
    }
    
    // Fazla büyük malloc kontrolü
    void* ptr = (void*)syscall_handler(SYS_MALLOC, 0x20000, 0, 0);
    if (ptr != NULL) {
        vga_print(MSG_FAIL, 50, 4, 0x0C);
        return TEST_FAIL;
    }
    
    vga_print(MSG_PASS, 50, 4, 0x0A);
    return TEST_PASS;
}

// Ana test fonksiyonu
void kernel_main() {
    vga_clear_screen();
    vga_print(MSG_START, 0, 0, 0x0F);
    
    // Basic syscall testleri
    test_result_t result = test_basic_syscalls();
    
    // Write syscall testleri
    result = test_write_syscall();
    
    // Memory syscall testleri
    result = test_memory_syscalls();
    
    // Process syscall testleri
    result = test_process_syscalls();
    
    // Security testleri
    result = test_security();
    
    // Test sonu
    vga_print(MSG_DONE, 0, 6, 0x0F);
    
    // Sonsuz döngü
    while (1) {
        __asm__ volatile("hlt");
    }
}
