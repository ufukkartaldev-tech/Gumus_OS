#include "ata.h"
#include "io.h"
#include "string.h"

// Internal functions
static void ata_wait_bsy() {
    while (inb(ATA_PRIMARY_COMM_STAT) & 0x80);
}

static void ata_wait_drq() {
    while (!(inb(ATA_PRIMARY_COMM_STAT) & 0x08));
}

void ata_read_sectors(uint32_t target_address, uint32_t LBA, uint8_t sector_count) {
    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((LBA >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, sector_count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)LBA);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(LBA >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(LBA >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x20); // Read sectors command

    uint16_t* target = (uint16_t*)target_address;

    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int i = 0; i < 256; i++) {
            target[i] = inw(ATA_PRIMARY_DATA);
        }
        target += 256;
    }
}

void ata_write_sectors(uint32_t LBA, uint8_t sector_count, uint32_t source_address) {
    ata_wait_bsy();
    outb(ATA_PRIMARY_DRIVE_SEL, 0xE0 | ((LBA >> 24) & 0x0F));
    outb(ATA_PRIMARY_SECCOUNT, sector_count);
    outb(ATA_PRIMARY_LBA_LO, (uint8_t)LBA);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(LBA >> 8));
    outb(ATA_PRIMARY_LBA_HI, (uint8_t)(LBA >> 16));
    outb(ATA_PRIMARY_COMM_STAT, 0x30); // Write sectors command

    uint16_t* source = (uint16_t*)source_address;

    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy();
        ata_wait_drq();
        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_DATA, source[i]);
        }
        source += 256;
    }
}

// Driver Interface Wrapper
static int ata_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    // Offset should be in bytes, convert to LBA
    // Size should be in bytes, convert to sectors
    uint32_t lba = offset / 512;
    uint32_t sectors = (size + 511) / 512; // Ceiling
    
    // Buffer should be valid pointer
    ata_read_sectors((uint32_t)buffer, lba, (uint8_t)sectors);
    return size;
}

static int ata_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    uint32_t lba = offset / 512;
    uint32_t sectors = (size + 511) / 512; // Ceiling
    
    ata_write_sectors(lba, (uint8_t)sectors, (uint32_t)buffer);
    return size;
}

static int ata_driver_init() {
    // ATA init logic if any (e.g. check identity)
    return 0;
}

static driver_t ata_driver = {
    .name = "ata_primary",
    .type = DRIVER_TYPE_BLOCK,
    .init = ata_driver_init,
    .read = ata_driver_read,
    .write = ata_driver_write,
    .ioctl = 0,
    .shutdown = 0
};

void ata_init_driver() {
    driver_register(&ata_driver);
}
