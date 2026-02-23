#include "idt.h"
#include "io.h"

struct idt_entry idt[IDT_ENTRIES];
struct idt_ptr idtp;

// assembly_handlers.asm dosyasÄ±ndan gelecek
extern void isr0();
extern void irq0();
extern void irq1();
extern void irq1();
extern void irq12();
extern void isr128(); // Syscall (int 0x80)

void set_idt_gate(int n, uint32_t handler) {
    idt[n].base_low = handler & 0xFFFF;
    idt[n].selector = 0x08; // KERNEL_CODE_SEGMENT (src/boot/gdt.asm'deki CODE_SEG)
    idt[n].always0 = 0;
    idt[n].flags = 0x8E; // Present, Ring 0, Interrupt Gate
    idt[n].base_high = (handler >> 16) & 0xFFFF;
}

// PIC (Programmable Interrupt Controller) Yeniden Haritalama
// BIOS varsayÄ±lan olarak IRQ'larÄ± 0x08-0x0F arasÄ±na atar, 
// ama bu Protected Mode'da CPU istisnalarÄ± ile Ã§akÄ±ÅŸÄ±r.
// Bu yÃ¼zden IRQ'larÄ± 0x20 (32) adresinden sonrasÄ±na taÅŸÄ±yoruz.
void remap_pic() {
    outb(0x20, 0x11); // Master PIC baÅŸlat
    outb(0xA0, 0x11); // Slave PIC baÅŸlat
    
    outb(0x21, 0x20); // Master IRQ offset -> 32 (0x20)
    outb(0xA1, 0x28); // Slave IRQ offset -> 40 (0x28)
    
    outb(0x21, 0x04); // Slave PIC'in baÄŸlÄ± olduÄŸu master hattÄ±
    outb(0xA1, 0x02); // Slave kimlik numarasÄ±
    
    outb(0x21, 0x01); // 8086 modu
    outb(0xA1, 0x01);
    
    outb(0x21, 0x0);  // TÃ¼m kesmeleri aÃ§
    
    // Slave PIC'te IRQ12 (Mouse) dÄ±ÅŸÄ±ndakileri maskelemek isteyebiliriz ama ÅŸimdilik aÃ§alÄ±m
    outb(0xA1, 0x0);
    
    // VarsayÄ±lan Maskeleme
    // Master PIC: IRQ0 (Timer), IRQ1 (Klavye), IRQ2 (Slave) AÃ§Ä±k -> 1111 1000 = 0xF8
    outb(0x21, 0xF8); 
    // Slave PIC: IRQ12 (Fare) AÃ§Ä±k -> 1110 1111 = 0xEF
    outb(0xA1, 0xEF);
}

void init_idt() {
    idtp.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idtp.base = (uint32_t)&idt;

    // TÃ¼m tabloyu sÄ±fÄ±rla
    for (int i = 0; i < IDT_ENTRIES; i++) {
        set_idt_gate(i, 0);
    }

    // Ã–rnek: Divide by Zero istisnasÄ± (0)
    set_idt_gate(0, (uint32_t)isr0);
    
    // IRQ'larÄ± yeniden haritala
    remap_pic();

    // IRQ 0: Timer
    set_idt_gate(32, (uint32_t)irq0);
    // IRQ 1: Klavye
    set_idt_gate(33, (uint32_t)irq1);
    
    // IRQ 12: Fare (Mouse)
    // IRQ 12: Fare (Mouse)
    set_idt_gate(44, (uint32_t)irq12);

    // Syscall (Ring 3 eriÅŸimine aÃ§Ä±k: 0xEE)
    idt[0x80].base_low = (uint32_t)isr128 & 0xFFFF;
    idt[0x80].selector = 0x08;
    idt[0x80].always0 = 0;
    idt[0x80].flags = 0xEE; // Present, Ring 3, Interrupt Gate
    idt[0x80].base_high = ((uint32_t)isr128 >> 16) & 0xFFFF;

    // IDT'yi yÃ¼kle
    __asm__ volatile("lidt %0" : : "m"(idtp));
}
