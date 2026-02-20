// GümüşOS - Port I/O Yardımcıları
#ifndef IO_H
#define IO_H

#include <stdint.h>

// Bir porttan 1 byte veri oku
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Bir porta 1 byte veri yaz
static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Bir porttan 2 byte (word) veri oku
static inline uint16_t inw(uint16_t port) {
    uint16_t result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Bir porta 2 byte (word) veri yaz
static inline void outw(uint16_t port, uint16_t data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

// Kısa bir bekleme (I/O gecikmesi için)
static inline void io_wait(void) {
    outb(0x80, 0);
}

// İşlemciyi bir sonraki kesmeye kadar durdur (Nefes aldır)
static inline void hlt(void) {
    __asm__ volatile("hlt");
}

#endif
