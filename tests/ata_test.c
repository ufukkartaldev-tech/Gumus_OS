#include "framework.h"
#include "../src/kernel/drivers/storage/ata.h"
#include "io.h"

/**
 * 1. KATMAN: ATA/IDE HARDWARE TEST (SÜRÜCÜ SEVİYESİ)
 */

// Mock Identify Data
static uint16_t identify_data[256];

// Dayı Tavsiyesi: PATA disklerde byte'lar ters gelir (Big-endian style strings in little-endian world)
static void swap_bytes(char* b, int len) {
    for (int i = 0; i < len; i += 2) {
        char tmp = b[i];
        b[i] = b[i + 1];
        b[i + 1] = tmp;
    }
}

int test_ata_port_presence() {
    // Dayı Tavsiyesi 1: Floating Bus Kontrolü
    // Eğer donanım yoksa veri yolu 0xFF döner. "Hayalet port" ile savaşma!
    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    ASSERT(status != 0xFF, "ATA Primary port not responding (Floating Bus / 0xFF detected)");
    return TEST_PASS;
}

int test_ata_identify_logic() {
    // Dayı Tavsiyesi 2: El Sıkışma (Handshaking)
    // Diske emir vermeden önce meşgul (BSY) olmadığından emin olmalısın.
    uint8_t status = inb(ATA_PRIMARY_COMM_STAT);
    ASSERT(!(status & 0x80), "ATA Busy: Disk is stuck in BSY before command");

    outb(ATA_PRIMARY_COMM_STAT, 0xEC); // IDENTIFY command
    
    // Status oku ve DRQ (Data Request) bekle
    status = inb(ATA_PRIMARY_COMM_STAT);
    // Mock sistemde 0xEC sonrası Status 0x08 (DRQ) olmalı.
    ASSERT(status & 0x08, "ATA Identify: Protocol error - DRQ bit not set after command");
    
    // 512 byte (256 words) okuma testi
    for(int i=0; i<256; i++) {
        identify_data[i] = inw(ATA_PRIMARY_DATA);
    }
    
    // Dayı Tavsiyesi 3: Byte Swapping (Endianness)
    // Model ismi word'ler içinde ters gelir.
    char model[41];
    memcpy(model, &identify_data[27], 40);
    model[40] = '\0';
    
    // Gerçek bir diskte swap_bytes(model, 40) gerekirdi.
    // Şimdilik verinin varlığını test ediyoruz.
    ASSERT(identify_data[27] != 0, "ATA Identify Data: Word 27 (Model) is zero or empty");
    
    return TEST_PASS;
}

void kernel_main() {
    test_header("1. LAYER: ATA/IDE HARDWARE VALIDATION");
    
    RUN_TEST(test_ata_port_presence, "I/O Port Presence (0x1F0-0x1F7)");
    RUN_TEST(test_ata_identify_logic, "ATA Identify Command & Data Integrity");
    
    _print_raw("Hardware ports verified.", 2, 10, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
