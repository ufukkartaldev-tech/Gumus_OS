; Disk okuma rutini
; BX: Verinin yükleneceği adres, DH: Sektör sayısı (opsiyonel, 32 varsayılan)
load_kernel:
    pusha
    
    ; Dayı Tavsiyesi: Kernel büyüdükçe burayı genişletmek lazım
    mov ah, 0x02    ; BIOS disk okuma fonksiyonu
    mov al, 32      ; 32 sektör oku (16KB) - Kernel için yeterli
    mov ch, 0x00    ; Silindir 0
    mov dh, 0x00    ; Kafa 0
    mov cl, 0x02    ; 2. sektörden başla (Bootloader 1. sektördür)
    ; DL is set by caller (BIOS provides boot drive in DL)
    int 0x13        ; BIOS Kesmesi
    
    jc .error       ; Hata kontrolü
    popa
    ret

.error:
    ; Hata durumunda bipleyebilir veya asılı kalabiliriz
    jmp $
