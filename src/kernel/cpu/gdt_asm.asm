[bits 32]
global gdt_flush
global tss_flush

gdt_flush:
    mov eax, [esp+4]  ; GDT pointer address
    lgdt [eax]        ; Load GDT
    
    mov ax, 0x10      ; Kernel Data Segment (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    jmp 0x08:.flush   ; Far jump to Kernel Code Segment (0x08)
.flush:
    ret

tss_flush:
    mov ax, 0x28      ; TSS Segment Selector (Index 5 -> 5*8 = 40 = 0x28)
    ltr ax            ; Load Task Register
    ret
