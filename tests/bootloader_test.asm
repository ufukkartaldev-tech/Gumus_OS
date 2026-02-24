; GümüşOS Bootloader Unit Test Suite (Gelişmiş v3)
; "Dayı Tavsiyesi" ile Reset/Retry, Segmentasyon ve Güvenli Çıktı eklendi.

[org 0x7c00]
[bits 16]

; --- BAŞLANGIÇ: Donanım ve Yığın Hazırlığı ---
start:
    cli                         ; Kesilmeleri durdur
    
    mov [BOOT_DRIVE], dl        ; BIOS'un verdiği boot disk numarasını sakla

    ; Segmentleri Garantile (Dayı Tavsiyesi 2)
    xor ax, ax                  
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Yığını 0x0000:0x7C00'a (kodun altına) kur
    
    sti                         ; Kesilmeleri tekrar aç

    ; Ekranı temizle
    call clear_screen

    ; Test Başlat Mesajı
    mov si, MSG_START
    call print_string

    ; --- TEST 1: Gerçek Disk Okuma (Reset & Retry - Dayı Tavsiyesi 1) ---
    call test_disk_read_with_retry

    ; Test Bitti
    mov si, MSG_DONE
    call print_string

    jmp $                       ; Sistemi durdur

; --- TEST FONKSİYONLARI ---

test_disk_read_with_retry:
    mov si, MSG_DISK_READ
    call print_string

    mov di, 3                   ; 3 deneme hakkı (Dayı Tavsiyesi 1)

.retry_loop:
    push di                     ; Deneme sayısını sakla
    
    ; Disk Sistemini Sıfırla (Reset)
    xor ah, ah
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc .next_retry              ; Sıfırlama hatası gelirse bir daha dene

    ; Sektör 2'yi Oku
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000              ; Veriyi 0x0000:0x8000 adresine oku
    
    mov ah, 0x02
    mov al, 1
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02
    mov dl, [BOOT_DRIVE]
    int 0x13
    
    jnc .read_success           ; Hata yoksa (CF=0) başarılı

.next_retry:
    pop di
    dec di
    jnz .retry_loop             ; Deneme hakkı kaldıysa tekrar dön
    
    ; 3 deneme de başarısız
    mov si, MSG_ERR_INT13
    call print_string
    call print_fail
    ret

.read_success:
    pop di                      ; Stack'i temizle
    
    ; Veriyi Doğrula (Sektör 2'nin başı 'D' olmalı)
    ; ds=0 olduğunu yukarıda garantiledik.
    mov al, [0x8000]
    cmp al, 'D'
    jne .data_error

    call print_pass
    ret

.data_error:
    mov si, MSG_ERR_DATA
    call print_string
    call print_fail
    ret

; --- YARDIMCI FONKSİYONLAR ---

clear_screen:
    pusha
    mov ax, 0x0600              ; Scroll up window (Clear)
    mov bh, 0x07                ; Grey on Black
    mov cx, 0x0000              ; Top Left
    mov dx, 0x184F              ; Bottom Right (80x25)
    int 0x10
    
    ; Cursor'ı resetle
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

; Güvenli Renkli Çıktı (Line-Wrap destekli - Dayı Tavsiyesi 3)
print_colored:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    
    ; 1. Karakteri ve Özniteliği (Attribute) yaz
    mov ah, 0x09
    mov bh, 0                   ; Sayfa 0
    mov cx, 1                   ; 1 karakter
    ; (AL = char, BL = color)
    int 0x10
    
    ; 2. İmleci ilerlet ve satır sonu kontrolü yap
    mov ah, 0x03                ; Get cursor pos
    xor bh, bh
    int 0x10                    ; (dl = col, dh = row)
    
    inc dl                      ; Bir sağa git
    cmp dl, 80                  ; Satır sonu mu?
    jne .set_pos
    
    xor dl, dl                  ; Sütun 0'a dön
    inc dh                      ; Alt satıra geç
    ; (Burada dh=25 kontrolü ve scrolling eklenebilir ama Bootloader için lüks)

.set_pos:
    mov ah, 0x02                ; Set cursor pos
    int 0x10
    
    jmp .loop
.done:
    popa
    ret

print_pass:
    push si
    mov si, MSG_PASS
    mov bl, 0x0A                ; Yeşil
    call print_colored
    pop si
    ret

print_fail:
    push si
    mov si, MSG_FAIL
    mov bl, 0x0C                ; Kırmızı
    call print_colored
    pop si
    ret

; --- VERİ BÖLGESİ ---
BOOT_DRIVE   db 0x00
MSG_START    db ">>> GUMUS OS BOOT TESTER v3.0 <<<", 0x0D, 0x0A, 0
MSG_DISK_READ db "Real Disk Read (Reset/Retry): ", 0
MSG_PASS     db " [PASS]", 0x0D, 0x0A, 0
MSG_FAIL     db " [FAIL]", 0x0D, 0x0A, 0
MSG_DONE     db 0x0D, 0x0A, "All system checks completed.", 0x0D, 0x0A, 0
MSG_ERR_INT13 db "(INT13 ERR) ", 0
MSG_ERR_DATA  db "(DATA ERR) ", 0

; Boot Signature
times 510-($-$$) db 0
dw 0xAA55

; --- SEKTÖR 2 (Test Verisi - Dayı Tavsiyesi 4 / Eklenti) ---
; NASM ile bin çıktısı alındığında bu veri 512. byte'tan sonra başlar
times 512 db 'D'
