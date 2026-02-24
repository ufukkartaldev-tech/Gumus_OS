; GümüşOS Protected Mode Unit Test (Gelişmiş v2)
; Mod geçişlerini (16->32->16) ve register güvenliğini test eder.

[org 0x7c00]
[bits 16]

start:
    cli                         ; Kesilmeleri durdur
    
    ; --- BAŞLANGIÇ: Donanım ve Yığın Hazırlığı ---
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; 16-bit Yığın

    call clear_screen

    ; Test Başlat Mesajı
    mov si, MSG_START
    call print_string

    ; --- TEST 1: GDT Kurulumu ---
    call test_gdt_setup

    ; --- TEST 2: Protected Mode Geçiş ve Geri Dönüş ---
    call test_mode_switch_rigorous

    ; --- TEST 3: Register Güvenliği (32-bit'te yapılır) ---
    ; (test_mode_switch_rigorous içinde 32-bit'teyken register testini de yapacağız)

    ; Test Bitti
    mov si, MSG_DONE
    call print_string

    jmp $

; --- TEST FONKSİYONLARI ---

test_gdt_setup:
    mov si, MSG_GDT
    call print_string
    
    lgdt [gdt_descriptor]
    
    ; SGDT ile doğrula
    sgdt [gdt_backup]
    mov ax, [gdt_backup]
    cmp ax, gdt_end - gdt_start - 1
    jne .fail
    
    call print_pass
    ret
.fail:
    call print_fail
    ret

test_mode_switch_rigorous:
    mov si, MSG_SWITCH
    call print_string

    ; --- 32-BIT'E GEÇİŞ (Dayı Tavsiyesi 3: Stack Hazır) ---
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1                   ; PE set
    mov cr0, eax

    jmp 0x08:init_pm            ; Far jump to 32-bit code

[bits 32]
init_pm:
    ; Registerları ayarla
    mov ax, 0x10                ; 32-bit Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000            ; Dayı Tavsiyesi 3: PM Stack

    ; Register Testi (Dayı Tavsiyesi 2)
    mov ebx, 0xCAFEBABE
    mov ecx, 0x12345678
    
    ; VGA'ya doğrudan erişim (Lineer Adres 0xB8000)
    mov edi, 0xB8000 + 160 * 6  ; 6. satır
    mov word [edi], 0x0F50      ; 'P'
    mov word [edi + 2], 0x0F4D  ; 'M'

    ; --- 16-BIT'E GERİ DÖNÜŞ (Dayı Tavsiyesi 1) ---
    ; Önce 16-bit uyumlu segmentleri yükle (Unreal Mode girişi)
    jmp 0x18:back_to_16_pm      ; Jump to 16-bit PM segment

[bits 16]
back_to_16_pm:
    mov ax, 0x20                ; 16-bit Data Segment
    mov ds, ax
    mov es, ax
    mov ss, ax

    ; PE bitini kapat
    mov eax, cr0
    and eax, ~1
    mov cr0, eax

    jmp 0x0000:back_to_real     ; Far jump back to Real Mode

back_to_real:
    ; Real mode segmentlerini temizle
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    call print_pass
    ret

; --- YARDIMCI FONKSİYONLAR (16-BIT) ---

clear_screen:
    pusha
    mov ax, 0x0700
    mov bh, 0x07
    mov cx, 0x0000
    mov dx, 0x184F
    int 0x10
    mov ah, 0x02
    xor bh, bh
    xor dx, dx
    int 0x10
    popa
    ret

print_string:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    or al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

print_pass:
    mov si, MSG_PASS
    call print_string
    call newline
    ret

print_fail:
    mov si, MSG_FAIL
    call print_string
    call newline
    ret

newline:
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    ret

; --- VERİ YAPILARI ---

align 16
gdt_start:
    dq 0x0                      ; Null
gdt_code_32:
    dq 0x00CF9A000000FFFF       ; 0x08: 32-bit Code
gdt_data_32:
    dq 0x00CF92000000FFFF       ; 0x10: 32-bit Data
gdt_code_16:
    dq 0x00009A000000FFFF       ; 0x18: 16-bit Code (Real Mode compatible)
gdt_data_16:
    dq 0x000092000000FFFF       ; 0x20: 16-bit Data (Real Mode compatible)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

gdt_backup:
    dw 0
    dd 0

MSG_START    db ">>> GUMUS OS PM TESTER v2.0 <<<", 0x0D, 0x0A, 0
MSG_GDT      db "GDT Setup: ", 0
MSG_SWITCH   db "16->32->16 Transition: ", 0
MSG_PASS     db "[PASS]", 0
MSG_FAIL     db "[FAIL]", 0
MSG_DONE     db 0x0D, 0x0A, "Testing Completed.", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55
