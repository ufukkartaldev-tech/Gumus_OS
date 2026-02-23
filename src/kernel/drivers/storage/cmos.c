#include "cmos.h"
#include "io.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

static uint8_t read_cmos_reg(uint8_t reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static int is_update_in_progress() {
    outb(CMOS_ADDR, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

void get_rtc_time(uint8_t *second, uint8_t *minute, uint8_t *hour) {
    while (is_update_in_progress());

    *second = read_cmos_reg(0x00);
    *minute = read_cmos_reg(0x02);
    *hour = read_cmos_reg(0x04);

    uint8_t status_b = read_cmos_reg(0x0B);

    // BCD'den Ã§evir (eÄŸer gerekliyse)
    if (!(status_b & 0x04)) {
        *second = (*second & 0x0F) + ((*second / 16) * 10);
        *minute = (*minute & 0x0F) + ((*minute / 16) * 10);
        *hour = ((*hour & 0x0F) + (((*hour & 0x70) / 16) * 10)) | (*hour & 0x80);
    }
    
    // 24 saat formatÄ± kontrolÃ¼
    if (!(status_b & 0x02) && (*hour & 0x80)) {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }
}
