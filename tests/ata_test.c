#include "framework.h"
#include "../src/kernel/drivers/storage/ata.h"
#include "io.h"

/**
 * 1. KATMAN: ATA/IDE HARDWARE TEST (SÜRÜCÜ SEVİYESİ)
 */

// Mock Identify Data
static uint16_t identify_data[256];

int test_ata_port_presence() {
    // Port 0x1F7 (Status) boşta 0xFF dönmemeli
    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    ASSERT(status != 0xFF, "ATA Primary port not responding (Floating Bus)");
    return TEST_PASS;
}

int test_ata_identify_logic() {
    // Identify komutu gönderildiğinde DRQ beklemeli
    // Mock sistemde outb command 0xEC yapıldığında DRQ set edilmeli
    outb(ATA_PRIMARY_COMM_STAT, 0xEC); // IDENTIFY
    
    // Status oku
    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    // Gerçekte DRQ (0x08) set edilmeli ama mock'ta manuel set edebiliriz
    ASSERT(status & 0x08, "ATA Identify: DRQ bit not set after command");
    
    // 512 byte okuma testi
    for(int i=0; i<256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_DATA);
    }
    
    // Model No (Sözde) kontrolü (Offset 27-46 in words)
    // Identify verisinde kelimeler ters byte sırasıyla gelir genellikle (PATA)
    // Ama mock'ta düzgün bir veri bekleyelim.
    ASSERT(identify_data[27] != 0, "ATA Identify: Model name is empty");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("1. LAYER: ATA/IDE HARDWARE VALIDATION");
    
    RUN_TEST(test_ata_port_presence, "I/O Port Presence (0x1F0-0x1F7)");
    RUN_TEST(test_ata_identify_logic, "ATA Identify Command & Data Integrity");
    
    _print_raw("Hardware ports verified.", 2, 10, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
