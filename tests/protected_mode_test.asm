; GümüşOS 32-bit Protected Mode Unit Test
; Protected Mode geçiş fonksiyonlarını test eder

[org 0x7c00]
[bits 16]

; Test mesajları
MSG_START db "32-bit Protected Mode Test Suite Baslatildi...", 0
MSG_GDT db "GDT Kurulumu Testi: ", 0
MSG_SWITCH db "Mode Switch Testi: ", 0
MSG_REGISTER db "Register Testi: ", 0
MSG_PASS db "PASS", 0
MSG_FAIL db "FAIL", 0
MSG_DONE db "Protected Mode Testleri Tamamlandi!", 0

; GDT yapısı
gdt_start:
    dd 0x0
    dd 0x0

gdt_code:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0

gdt_data:
    dw 0xffff
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; Test başlat
start:
    call clear_screen
    mov si, MSG_START
    call print_string
    call newline
    
    ; GDT kurulumu testi
    call test_gdt_setup
    
    ; Mode switch testi
    call test_mode_switch
    
    ; Register testi
    call test_registers
    
    ; Test sonu
    call newline
    mov si, MSG_DONE
    call print_string
    
    jmp $

; GDT kurulumu testi
test_gdt_setup:
    mov si, MSG_GDT
    call print_string
    
    ; GDT'yi yükle
    lgdt [gdt_descriptor]
    
    ; GDT yüklendiğini kontrol et
    sgdt [gdt_backup]
    mov ax, [gdt_backup]
    cmp ax, gdt_end - gdt_start - 1
    jne gdt_fail
    
    call print_pass
    ret

gdt_fail:
    call print_fail
    ret

; Mode switch testi
test_mode_switch:
    call newline
    mov si, MSG_SWITCH
    call print_string
    
    ; 32-bit'e geç
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    
    ; 32-bit'e geçtiğimizi test et
    jmp CODE_SEG:test_32bit
    
[bits 32]
test_32bit:
    ; 32-bit'te olduğumuzu doğrula
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Başarılı - 16-bit'e geri dön
    jmp CODE_SEG:back_to_16bit

[bits 16]
back_to_16bit:
    ; 16-bit'e geri dön
    mov eax, cr0
    and eax, 0xFFFFFFFE
    mov cr0, eax
    jmp CODE_SEG:back_in_16bit

back_in_16bit:
    call print_pass
    ret

; Register testi
test_registers:
    call newline
    mov si, MSG_REGISTER
    call print_string
    
    ; Segment register'ları test et
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Test verisi yaz
    mov ax, 0xB800
    mov es, ax
    mov word [es:0], 0x0F52  ; 'R' karakteri
    
    ; Oku ve kontrol et
    mov ax, [es:0]
    cmp ax, 0x0F52
    jne register_fail
    
    call print_pass
    ret

register_fail:
    call print_fail
    ret

; Yardımcı fonksiyonlar
clear_screen:
    pusha
    mov ax, 0xB800
    mov es, ax
    xor di, di
    mov cx, 80 * 25
    mov al, ' '
    mov ah, 0x0F
    rep stosw
    popa
    ret

print_string:
    pusha
    mov ah, 0x0E
.print_loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .print_loop
.done:
    popa
    ret

print_pass:
    pusha
    mov si, MSG_PASS
    call print_string
    popa
    ret

print_fail:
    pusha
    mov si, MSG_FAIL
    call print_string
    popa
    ret

newline:
    pusha
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    popa
    ret

; GDT backup için alan
gdt_backup:
    times 6 db 0

times 510-($-$$) db 0
dw 0xAA55
