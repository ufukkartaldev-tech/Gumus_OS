// GümüşOS - Kesme Tanımlayıcı Tablosu (IDT) Tanımları
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

// IDT Giriş Yapısı (Gate Descriptor)
struct idt_entry {
    uint16_t base_low;  // Kesme işleyici adresinin alt 16 biti
    uint16_t selector;  // GDT'deki Code Segment seçicisi
    uint8_t  always0;   // Her zaman 0 olmalı
    uint8_t  flags;     // Bayraklar (P, DPL, Type)
    uint16_t base_high; // Kesme işleyici adresinin üst 16 biti
} __attribute__((packed));

// lidt komutu için gerekli yapı
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void init_idt();
void set_idt_gate(int n, uint32_t handler);

#endif
