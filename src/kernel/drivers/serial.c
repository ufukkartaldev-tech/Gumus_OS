#include "serial.h"
#include "../core/io.h"
#include "../core/string.h"

int serial_init() {
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    
    // Check if serial is faulty (try loopback)
    outb(COM1_PORT + 4, 0x1E);    // Set in loopback mode
    outb(COM1_PORT + 0, 0xAE);    // Test serial chip
    
    if(inb(COM1_PORT + 0) != 0xAE) {
        return -1;
    }
    
    // If serial is not faulty, set it in normal operation mode
    outb(COM1_PORT + 4, 0x0F);
    return 0;
}

int serial_received() {
    return inb(COM1_PORT + 5) & 1;
}

char serial_read() {
    while (serial_received() == 0);
    return inb(COM1_PORT + 0);
}

int is_transmit_empty() {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putchar(char c) {
    while (is_transmit_empty() == 0);
    outb(COM1_PORT, c);
}

void serial_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putchar(str[i]);
    }
}

// Driver Manager Wrapper
static int serial_driver_init(void) {
    return serial_init();
}

static int serial_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    char* buf = (char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = serial_read();
    }
    return size;
}

static int serial_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    const char* buf = (const char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        serial_putchar(buf[i]);
    }
    return size;
}

static driver_t serial_driver = {
    .name = "serial_com1",
    .type = DRIVER_TYPE_CHAR,
    .class = PCI_CLASS_SERIAL,
    .init = serial_driver_init,
    .read = serial_driver_read,
    .write = serial_driver_write,
    .ioctl = 0,
    .shutdown = 0
};

driver_t* create_serial_driver() {
    return &serial_driver;
}
