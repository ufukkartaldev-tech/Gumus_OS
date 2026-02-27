#include <stdint.h>

void ata_read_sectors(uint32_t target_address, uint32_t LBA, uint8_t sector_count) {
    static uint8_t disk[256 * 1024]; // 256KB sahte disk
    (void)LBA;
    for (uint32_t i = 0; i < (uint32_t)sector_count * 512; i++) {
        ((uint8_t*)target_address)[i] = disk[i];
    }
}

void ata_write_sectors(uint32_t LBA, uint8_t sector_count, uint32_t source_address) {
    static uint8_t disk[256 * 1024]; // 256KB sahte disk
    (void)LBA;
    for (uint32_t i = 0; i < (uint32_t)sector_count * 512; i++) {
        disk[i] = ((uint8_t*)source_address)[i];
    }
}
