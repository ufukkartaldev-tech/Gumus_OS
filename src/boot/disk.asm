; Disk okuma rutini - en basit hali
; BX: Verinin yükleneceği adres, DH: Okunacak sektör sayısı
load_kernel:
    pusha
    
    ; En basit disk okuma - sadece 1 sektör
    mov ah, 0x02    ; BIOS disk okuma fonksiyonu
    mov al, 1       ; Sadece 1 sektör oku
    mov ch, 0x00    ; Silindir 0
    mov dh, 0x00    ; Kafa 0
    mov cl, 0x02    ; 2. sektörden başla
    int 0x13        ; BIOS Kesmesi
    
    popa
    ret
