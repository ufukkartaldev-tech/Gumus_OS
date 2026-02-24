; GümüşOS VGA Text Mode Unit Test (Dayı Tavsiyesi v2)
; VGA yazma, okuma ve scroll fonksiyonlarını doğru segmentasyon ile test eder.

[org 0x7c00]
[bits 16]

; Dayı Tavsiyesi 2: VGA_ADDR artık Segment adresi
VGA_SEG   equ 0xB800
VGA_WIDTH equ 80
VGA_HEIGHT equ 25

start:
    ; Dayı Tavsiyesi 1: Segmentleri Mühürle
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Dayı Tavsiyesi 4: ES'yi bir kez mühürle, hamallığı bırak
    mov ax, VGA_SEG
    mov es, ax

    call clear_screen
    
    mov si, MSG_START
    call print_string
    call newline
    
    ; --- TEST 1: Ekran Temizleme ---
    call test_clear_screen
    
    ; --- TEST 2: Karakter Yazma ---
    call test_char_write
    
    ; --- TEST 3: Scroll (Kaydırma) ---
    call test_scroll
    
    mov si, MSG_DONE
    call print_string
    
    jmp $

; --- TEST FONKSİYONLARI ---

test_clear_screen:
    mov si, MSG_CLEAR
    call print_string
    
    call fill_screen
    call clear_screen
    
    ; ES:DI üzerinden kontrol (DI=0 çünkü ES=0xB800)
    xor di, di
    mov cx, VGA_WIDTH * VGA_HEIGHT
.loop:
    mov al, [es:di]
    cmp al, ' '
    jne .fail
    add di, 2
    loop .loop
    
    call print_pass
    ret
.fail:
    call print_fail
    ret

test_char_write:
    call newline
    mov si, MSG_WRITE
    call print_string
    
    ; 2. satır (160 offset)
    mov di, 160
    mov ax, 0x0F41  ; 'A', White on Black
    stosw
    mov ax, 0x0F42  ; 'B'
    stosw
    
    xor di, di
    add di, 160
    mov al, [es:di]
    cmp al, 'A'
    jne .fail
    mov al, [es:di+2]
    cmp al, 'B'
    jne .fail
    
    call print_pass
    ret
.fail:
    call print_fail
    ret

test_scroll:
    call newline
    mov si, MSG_SCROLL
    call print_string
    
    call fill_screen    ; Her yer 'X'
    call scroll_up      ; 2. satırdaki 'X'ler 1. satıra gelmeli
    
    xor di, di
    mov al, [es:di]
    cmp al, 'X'
    jne .fail
    
    call print_pass
    ret
.fail:
    call print_fail
    ret

; --- YARDIMCI FONKSİYONLAR ---

clear_screen:
    pusha
    xor di, di
    mov cx, VGA_WIDTH * VGA_HEIGHT
    mov ax, 0x0F20      ; Space, White on Black
    rep stosw
    popa
    ret

fill_screen:
    pusha
    xor di, di
    mov cx, VGA_WIDTH * VGA_HEIGHT
    mov ax, 0x0F58      ; 'X', White on Black
    rep stosw
    popa
    ret

scroll_up:
    pusha
    ; Dayı Tavsiyesi 3: DS'yi geçici olarak VGA segmentine çek
    push ds
    mov ax, VGA_SEG
    mov ds, ax          ; Kaynak: VGA
    mov es, ax          ; Hedef: VGA
    
    mov si, VGA_WIDTH * 2  ; 2. satır (Kaynak)
    mov di, 0              ; 1. satır (Hedef)
    mov cx, (VGA_HEIGHT - 1) * VGA_WIDTH
    rep movsw
    
    ; Son satırı temizle
    mov di, (VGA_HEIGHT - 1) * VGA_WIDTH * 2
    mov cx, VGA_WIDTH
    mov ax, 0x0F20
    rep stosw
    
    pop ds              ; DS'yi geri al (Stringler için lazım)
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
    ret

print_fail:
    mov si, MSG_FAIL
    call print_string
    ret

newline:
    mov ah, 0x0E
    mov al, 0x0D
    int 0x10
    mov al, 0x0A
    int 0x10
    ret

MSG_START db ">>> GUMUS OS VGA TESTER v2.0 <<<", 0x0D, 0x0A, 0
MSG_CLEAR db "Clear Screen: ", 0
MSG_WRITE db "Char Write: ", 0
MSG_SCROLL db "Scroll Up: ", 0
MSG_PASS  db "[PASS]", 0
MSG_FAIL  db "[FAIL]", 0
MSG_DONE  db 0x0D, 0x0A, "VGA Matrix Verified.", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55
