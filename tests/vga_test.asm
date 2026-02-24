; GümüşOS VGA Text Mode Unit Test
; VGA yazma fonksiyonlarını test eder

[org 0x7c00]
[bits 16]

; VGA adresleri
VGA_ADDR equ 0xB8000
VGA_WIDTH equ 80
VGA_HEIGHT equ 25

; Test mesajları
MSG_START db "VGA Text Mode Test Suite Baslatildi...", 0
MSG_CLEAR db "Ekran Temizleme Testi: ", 0
MSG_WRITE db "Karakter Yazma Testi: ", 0
MSG_SCROLL db "Scroll Testi: ", 0
MSG_PASS db "PASS", 0
MSG_FAIL db "FAIL", 0
MSG_DONE db "VGA Testleri Tamamlandi!", 0

; Test başlat
start:
    call clear_screen
    mov si, MSG_START
    call print_string
    call newline
    
    ; Ekran temizleme testi
    call test_clear_screen
    
    ; Karakter yazma testi
    call test_char_write
    
    ; Scroll testi
    call test_scroll
    
    ; Test sonu
    call newline
    mov si, MSG_DONE
    call print_string
    
    jmp $

; Ekran temizleme testi
test_clear_screen:
    mov si, MSG_CLEAR
    call print_string
    
    ; Ekranı doldur
    call fill_screen
    ; Ekranı temizle
    call clear_screen
    
    ; Temizlendiğini kontrol et
    mov di, VGA_ADDR
    mov cx, VGA_WIDTH * VGA_HEIGHT
.check_loop:
    mov al, [es:di]
    cmp al, ' '
    jne clear_fail
    add di, 2
    loop .check_loop
    
    call print_pass
    ret

clear_fail:
    call print_fail
    ret

; Karakter yazma testi
test_char_write:
    call newline
    mov si, MSG_WRITE
    call print_string
    
    ; Test karakterleri yaz
    mov di, VGA_ADDR + 160  ; 2. satır
    mov al, 'A'
    mov ah, 0x0F
    stosw
    mov al, 'B'
    stosw
    mov al, 'C'
    stosw
    
    ; Yazıldığını kontrol et
    mov di, VGA_ADDR + 160
    mov al, [es:di]
    cmp al, 'A'
    jne write_fail
    add di, 2
    mov al, [es:di]
    cmp al, 'B'
    jne write_fail
    add di, 2
    mov al, [es:di]
    cmp al, 'C'
    jne write_fail
    
    call print_pass
    ret

write_fail:
    call print_fail
    ret

; Scroll testi
test_scroll:
    call newline
    mov si, MSG_SCROLL
    call print_string
    
    ; Ekranı doldur
    call fill_screen
    
    ; Scroll yap
    call scroll_up
    
    ; Scroll kontrolü
    mov di, VGA_ADDR
    mov al, [es:di]
    cmp al, 'X'
    jne scroll_fail
    
    call print_pass
    ret

scroll_fail:
    call print_fail
    ret

; Yardımcı fonksiyonlar
clear_screen:
    pusha
    mov ax, VGA_ADDR
    mov es, ax
    xor di, di
    mov cx, VGA_WIDTH * VGA_HEIGHT
    mov al, ' '
    mov ah, 0x0F
    rep stosw
    popa
    ret

fill_screen:
    pusha
    mov ax, VGA_ADDR
    mov es, ax
    xor di, di
    mov cx, VGA_WIDTH * VGA_HEIGHT
    mov al, 'X'
    mov ah, 0x0F
    rep stosw
    popa
    ret

scroll_up:
    pusha
    mov ax, VGA_ADDR
    mov es, ax
    mov si, VGA_WIDTH * 2  ; 2. satırdan başla
    mov di, 0              ; 1. satıra yaz
    mov cx, (VGA_HEIGHT - 1) * VGA_WIDTH
    rep movsw
    
    ; Son satırı temizle
    mov di, (VGA_HEIGHT - 1) * VGA_WIDTH * 2
    mov cx, VGA_WIDTH
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
