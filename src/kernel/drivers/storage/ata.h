#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#include "driver.h"

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_SEL    0x1F6
#define ATA_PRIMARY_COMM_STAT    0x1F7

void ata_read_sectors(uint32_t target_address, uint32_t LBA, uint8_t sector_count);
void ata_write_sectors(uint32_t LBA, uint8_t sector_count, uint32_t source_address);
void ata_init_driver();

#endif
