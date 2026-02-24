; GümüşOS IDT & Protected Mode Test (Layer 0 - Pro)
; Dayı Tavsiyesi: 16-bit'ten 32-bit'e geçiş ve IDT tapu kontrolü.

[org 0x7c00]
[bits 16]

start:
    cli                         ; 1. Kural: Kesmeleri durdur (Dayı Tavsiyesi - Stack tehlikesi)

    ; --- 16-BIT HAZIRLIK ---
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; --- GDT KURULUMU (32-bit'e Geçiş Bileti) ---
    lgdt [gdt_descriptor]

    ; CR0 register'ında PE (Protection Enable) bitini set et
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Uzun atlama (Far Jump) ile segmentleri tazele ve 32-bit moduna geç
    jmp 0x08:init_pm

[bits 32]
init_pm:
    ; 2. Kural: 32-bit Segmentlerini (Data) ayarla
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000            ; Güvenli bir 32-bit stack alanı (Dayı Tavsiyesi 4)

    ; --- IDT TAPU KAYDI (Dayı Tavsiyesi 1 & 3) ---
    call setup_idt
    lidt [mini_idt_descriptor]

    ; VGA'ya "PM OK" bas (0xB8000 lineer adres)
    mov dword [0xB8000], 0x0F4D0F50 ; 'P', 'M' (Beyaz/Siyah)

    ; --- TEST: INT 0x80 TETİKLE ---
    int 0x80

    ; Eğer buraya döndüysek test başarılı (iretd çalıştı)
    mov dword [0xB8008], 0x0A4B0A4F ; 'O', 'K' (Yeşil)

    jmp $

setup_idt:
    ; Dayı Tavsiyesi 3: IDT Gate yapısını elle (Bit bit) doldur
    ; int 0x80 (128. giriş) -> 128 * 8 = 1024 (0x400) offset
    mov eax, test_handler
    mov [mini_idt + 0x400], ax       ; Offset low (0-15)
    mov word [mini_idt + 0x400 + 2], 0x08 ; Selector (Kernel Code)
    mov byte [mini_idt + 0x400 + 4], 0    ; Reserved
    mov byte [mini_idt + 0x400 + 5], 0xEE ; Access: Present, Ring 3, IntGate (Dayı Tavsiyesi 3)
    shr eax, 16
    mov [mini_idt + 0x400 + 6], ax   ; Offset high (16-31)
    ret

test_handler:
    ; Dayı Tavsiyesi 4: Stack'te Flags, CS, EIP var. 
    ; VGA'ya "INT" yaz ki çalıştığını anlayalım
    mov dword [0xB8010], 0x0E540E4E0E49 ; 'I', 'N', 'T' (Sarı)
    iretd                       ; 32-bit Interrupt Return (Geri Dön!)

; --- VERİ YAPILARI ---

align 8
gdt_start:
    dq 0x0                      ; Null Descriptor
gdt_code:
    dq 0x00CF9A000000FFFF       ; Code Segment (0x08)
gdt_data:
    dq 0x00CF92000000FFFF       ; Data Segment (0x10)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 16
mini_idt:
    times 256 dq 0              ; 256 adet 8-byte giriş (Dayı Tavsiyesi 5 - Hiza)

mini_idt_descriptor:
    dw 256 * 8 - 1
    dd mini_idt                 ; Protected Mode'da bu lineer adrestir!

times 510-($-$$) db 0
dw 0xAA55