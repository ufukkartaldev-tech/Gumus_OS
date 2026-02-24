; GümüşOS IDT (Interrupt Descriptor Table) Unit Test
; Interrupt yönetimini test eder

[org 0x7c00]
[bits 16]

; Test mesajları
MSG_START db "IDT Test Suite Baslatildi...", 0
MSG_IDT_SETUP db "IDT Kurulumu Testi: ", 0
MSG_INTERRUPT db "Interrupt Testi: ", 0
MSG_PASS db "PASS", 0
MSG_FAIL db "FAIL", 0
MSG_DONE db "IDT Testleri Tamamlandi!", 0

; IDT yapısı
idt:
    times 256 dq 0  ; 256 interrupt için

idt_descriptor:
    dw idt_end - idt - 1  ; IDT boyutu
    dd idt               ; IDT adresi

; Test interrupt handler
test_handler:
    pusha
    mov ax, 0xB800
    mov es, ax
    mov word [es:160], 0x0F49  ; 'I' karakteri
    popa
    iret

idt_end:

; Test başlat
start:
    call clear_screen
    mov si, MSG_START
    call print_string
    call newline
    
    ; IDT kurulumu testi
    call test_idt_setup
    
    ; Interrupt testi
    call test_interrupt
    
    ; Test sonu
    call newline
    mov si, MSG_DONE
    call print_string
    
    jmp $

; IDT kurulumu testi
test_idt_setup:
    mov si, MSG_IDT_SETUP
    call print_string
    
    ; IDT'yi yükle
    lidt [idt_descriptor]
    
    ; Test interrupt handler'ını ayarla
    mov eax, test_handler
    mov [idt + 0x80 * 8], ax
    shr eax, 16
    mov [idt + 0x80 * 8 + 6], ax
    
    call print_pass
    ret

; Interrupt testi
test_interrupt:
    call newline
    mov si, MSG_INTERRUPT
    call print_string
    
    ; Ekranı temizle
    mov ax, 0xB800
    mov es, ax
    mov word [es:160], 0x0F20  ; Boşluk
    
    ; Test interrupt'ı çağır
    int 0x80
    
    ; Sonucu kontrol et
    mov ax, [es:160]
    cmp ax, 0x0F49
    jne interrupt_fail
    
    call print_pass
    ret

interrupt_fail:
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

times 510-($-$$) db 0
dw 0xAA55
