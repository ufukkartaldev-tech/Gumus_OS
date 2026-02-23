#include "gdt.h"
#include "string.h"

// GDT Entries: Null, KCode, KData, UCode, UData, TSS
// Total: 6 entries
gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

// Assembly'de tanÄ±mlÄ± GDT yÃ¼kleme fonksiyonu
extern void gdt_flush(uint32_t);
extern void tss_flush(); // LTR komutu

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= (gran & 0xF0);
    gdt_entries[num].access      = access;
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t) &tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    gdt_set_gate(num, base, limit, 0xE9, 0x00); // Present, Ring 3, TSS(0x9)

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0  = ss0;  // Kernel Stack Segment
    tss_entry.esp0 = esp0; // Kernel Stack Pointer (Initial)

    // CS, SS, DS, ES, FS, GS
    tss_entry.cs   = 0x08 | 0x3; // Kernel Code Segment but logical OR with 3? No actually used only for hardware switching which we don't use.
    // Actually simpler:
    tss_entry.ss0  = ss0;
    tss_entry.esp0 = esp0;
    
    // IOPL allow access to hardware ports in user mode? No.
    // But setting IOMap base to sizeof(tss) disables bitmap.
    tss_entry.iomap_base = sizeof(tss_entry);
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code segment (0x08)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data segment (0x10)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code segment (0x18) - Ring 3
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data segment (0x20) - Ring 3
    
    // TSS Segment (0x28)
    // Åimdilik stack 0, context switch sÄ±rasÄ±nda gÃ¼ncellenecek
    write_tss(5, 0x10, 0); 

    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush(); // 0x28 | 3 ? No, TSS selector is just index 5 -> 0x28
}

// Context Switch sÄ±rasÄ±nda Kernel Stack'i gÃ¼ncellemek iÃ§in
void tss_set_stack(uint32_t kernel_ss, uint32_t kernel_esp) {
    tss_entry.ss0 = kernel_ss;
    tss_entry.esp0 = kernel_esp;
}
