; GümüşOS - GDT (Global Descriptor Table)
; Hafıza segmentlerini tanımlayan tablo

gdt_start:
    ; Boş giriş (Null Descriptor) - Olması zorunludur
    dd 0x0
    dd 0x0

gdt_code:
    ; Kod Segmenti Tanımlayıcısı
    ; base=0x0, limit=0xfffff
    ; 1st flags: (present)1 (privilege)00 (type)1 -> 1001b
    ; type flags: (code)1 (conforming)0 (readable)1 (accessed)0 -> 1010b
    ; 2nd flags: (granularity)1 (32-bit default)1 (64-bit seg)0 (AVL)0 -> 1100b
    dw 0xffff       ; Limit (bits 0-15)
    dw 0x0          ; Base (bits 0-15)
    db 0x0          ; Base (bits 16-23)
    db 10011010b    ; 1st flags + type flags
    db 11001111b    ; 2nd flags + Limit (bits 16-19)
    db 0x0          ; Base (bits 24-31)

gdt_data:
    ; Veri Segmenti Tanımlayıcısı
    ; type flags: (code)0 (expand down)0 (writable)1 (accessed)0 -> 0010b
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_code16:
    ; 16-bit Kod Segmenti (Real Mode uyumlu)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 00001111b    ; Granularity=0, 32-bit default=0
    db 0x0

gdt_data16:
    ; 16-bit Veri Segmenti (Real Mode uyumlu)
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 00001111b    ; Granularity=0, 32-bit default=0
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; GDT Boyutu
    dd gdt_start                ; GDT Başlangıç Adresi

; Segment ofsetleri (Kernel için gereklidir)
CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
CODE16_SEG equ gdt_code16 - gdt_start
DATA16_SEG equ gdt_data16 - gdt_start
