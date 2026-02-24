; GümüşOS IDT Mini Test
; Basit interrupt testi

[org 0x7c00]
[bits 16]

; Test mesajları
MSG_START db "IDT Mini Test", 0
MSG_PASS db "PASS", 0
MSG_FAIL db "FAIL", 0

; Mini IDT (sadece 8 entry)
mini_idt:
    times 8 dq 0

mini_idt_descriptor:
    dw mini_idt_end - mini_idt - 1
    dd mini_idt

; Test handler
test_handler:
    pusha
    mov ax, 0xB800
    mov es, ax
    mov word [es:160], 0x0F49  ; 'I' karakteri
    popa
    iret

mini_idt_end:

; Test başlat
start:
    mov si, MSG_START
    call print_string
    call newline
    
    ; IDT'yi yükle
    lidt [mini_idt_descriptor]
    
    ; Handler'ı ayarla
    mov eax, test_handler
    mov [mini_idt + 0x80 * 8], ax
    shr eax, 16
    mov [mini_idt + 0x80 * 8 + 6], ax
    
    ; Interrupt test
    int 0x80
    
    ; Sonucu kontrol et
    mov ax, 0xB800
    mov es, ax
    mov ax, [es:160]
    cmp ax, 0x0F49
    je test_pass
    
    mov si, MSG_FAIL
    call print_string
    jmp $
    
test_pass:
    call newline
    mov si, MSG_PASS
    call print_string
    jmp $

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
