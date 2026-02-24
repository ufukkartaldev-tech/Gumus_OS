; GümüşOS User Mode & Security (GPF) Test
; "Dayı Tavsiyesi": Ring 3'te yasaklı komutların sistem tarafından durdurulmasını test eder.

[org 0x7c00]
[bits 16]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; 32-bit PM Geçişi
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp 0x08:init_pm_32

[bits 32]
init_pm_32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; IDT Kurulumu (GPF İşleyici ile)
    call setup_gpf_idt
    lidt [idt_descriptor]

    ; Ekran Hazırlığı
    call clear_vga
    mov esi, MSG_START
    mov edi, 0xB8000 + 160 * 1 + 10
    mov ah, 0x0E
    call print_vga

    ; --- TEST: RING 3 GEÇİŞİ VE İHLAL ---
    ; User Stack Hazırla
    push 0x23               ; SS (User Data + RPL 3)
    push 0x80000            ; ESP
    pushfd                  ; EFLAGS
    push 0x1B               ; CS (User Code + RPL 3)
    push ring3_code         ; EIP
    
    iret                    ; FLY TO RING 3!

ring3_code:
    ; ŞİMDİ USER MODNDAYIZ (Ring 3)
    
    ; YASAKLI KOMUT TESTİ: CLI (General Protection Fault Tetiklemeli)
    cli                     ; BOOM! (Exception 13 expected)
    
    ; Eğer buraya ulaşırsak test BAŞARISIZDIR
    mov esi, MSG_FAIL
    mov edi, 0xB8000 + 160 * 5 + 10
    mov ah, 0x0C
    call print_vga
    jmp $

gpf_handler:
    ; GPF Geldi! Test BAŞARILI.
    mov esi, MSG_PASS
    mov edi, 0xB8000 + 160 * 5 + 10
    mov ah, 0x0A
    call print_vga
    
    mov esi, MSG_GPF_ALERT
    mov edi, 0xB8000 + 160 * 6 + 10
    mov ah, 0x0E
    call print_vga
    
    jmp $ ; Test bitti, sistemi durdur.

; --- YARDIMCI FONKSİYONLAR ---

setup_gpf_idt:
    ; GPF (Exception 13) Kapısını kur
    mov eax, gpf_handler
    mov [idt + 13 * 8], ax
    mov word [idt + 13 * 8 + 2], 0x08
    mov byte [idt + 13 * 8 + 4], 0
    mov byte [idt + 13 * 8 + 5], 0x8E ; Present, Ring 0, Interrupt Gate
    shr eax, 16
    mov [idt + 13 * 8 + 6], ax
    ret

clear_vga:
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0F20
    rep stosw
    ret

print_vga:
.loop:
    lodsb
    or al, al
    jz .done
    mov [edi], al
    mov [edi+1], ah
    add edi, 2
    jmp .loop
.done:
    ret

; --- VERİ YAPILARI ---

align 16
gdt_start:
    dq 0x0                  ; Null
    dq 0x00CF9A000000FFFF   ; 0x08: Kernel Code
    dq 0x00CF92000000FFFF   ; 0x10: Kernel Data
    dq 0x00CFFA000000FFFF   ; 0x18: User Code (Ring 3)
    dq 0x00CFF2000000FFFF   ; 0x20: User Data (Ring 3)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 4
idt:
    times 16 dq 0           ; GPF is 13, so 16 entries is enough
idt_descriptor:
    dw 16 * 8 - 1
    dd idt

MSG_START     db "UserMode Security Test", 0
MSG_PASS      db "PASS: [GPF TRAP OK]", 0
MSG_FAIL      db "FAIL: RING 3 BREACH", 0
MSG_GPF_ALERT db "Blocked cli in Ring 3", 0

times 510-($-$$) db 0
dw 0xAA55
