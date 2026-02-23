#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

void get_rtc_time(uint8_t *second, uint8_t *minute, uint8_t *hour);

#endif
