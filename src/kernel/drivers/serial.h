#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include "driver.h"

// COM Ports
#define COM1_PORT 0x3F8
#define COM2_PORT 0x2F8

// Serial Register Offsets
#define SERIAL_DATA_REG          0
#define SERIAL_INTERRUPT_ENABLE  1
#define SERIAL_BAUD_LOW          0
#define SERIAL_BAUD_HIGH         1
#define SERIAL_FIFO_CTRL         2
#define SERIAL_LINE_CTRL         3
#define SERIAL_MODEM_CTRL         4
#define SERIAL_LINE_STATUS       5
#define SERIAL_MODEM_STATUS      6
#define SERIAL_SCRATCH           7

// Serial Initialization
int serial_init();
void serial_putchar(char c);
void serial_print(const char* str);
int serial_received();
char serial_read();

// Driver Manager Interface
driver_t* create_serial_driver();

#endif
