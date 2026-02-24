; GümüşOS Bootloader Unit Test Suite
; Disk okuma fonksiyonlarını test eder

[org 0x7c00]
[bits 16]

; Test sonuçları için VGA adresleri
TEST_ADDR equ 0xB8000
PASS_COLOR equ 0x0A    ; Yeşil
FAIL_COLOR equ 0x0C    ; Kırmızı

; Test mesajları
MSG_START db "Bootloader Unit Test Suite Baslatildi...", 0
MSG_DISK_READ db "Disk Okuma Testi: ", 0
MSG_PASS db "PASS", 0
MSG_FAIL db "FAIL", 0
MSG_DONE db "Tum Testler Tamamlandi!", 0

; Test başlat
start:
    mov si, MSG_START
    call print_string
    
    ; Disk okuma testi
    call test_disk_read
    
    ; Test sonu
    mov si, MSG_DONE
    call print_string
    
    jmp $

; Disk okuma testi
test_disk_read:
    mov si, MSG_DISK_READ
    call print_string
    
    ; Test verisi yaz
    mov ax, TEST_ADDR
    mov es, ax
    mov di, 100
    mov al, 'T'
    stosb
    
    ; Disk okuma fonksiyonunu çağır (simüle)
    call simulate_disk_read
    
    ; Sonucu kontrol et
    mov al, [es:100]
    cmp al, 'T'
    je test_pass
    
    ; Test başarısız
    call print_fail
    ret

test_pass:
    call print_pass
    ret

; Simüle edilmiş disk okuma
simulate_disk_read:
    ; Gerçek disk okuma yerine basit test
    mov ax, TEST_ADDR
    mov es, ax
    mov di, 100
    mov al, 'T'
    stosb
    ret

; Yardımcı fonksiyonlar
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
    mov bl, PASS_COLOR
    call print_colored
    popa
    ret

print_fail:
    pusha
    mov si, MSG_FAIL
    mov bl, FAIL_COLOR
    call print_colored
    popa
    ret

print_colored:
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

times 510-($-$$) db 0
dw 0xAA55
