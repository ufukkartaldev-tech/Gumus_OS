; Disk okuma rutini
; BX: Verinin yükleneceği adres, DH: Okunacak sektör sayısı
load_kernel:
    pusha
    push dx

    mov ah, 0x02    ; BIOS disk okuma fonksiyonu
    mov al, dh      ; Sektör sayısı
    mov ch, 0x00    ; Silindir (Cylinder) 0
    mov dh, 0x00    ; Kafa (Head) 0
    mov cl, 0x02    ; 2. sektörden başla (bootloader 1. sektördür)
    
    ; DL zaten boot drive numarasını içeriyor (BIOS'tan gelen)
    int 0x13        ; BIOS Kesmesi

    jc disk_error   ; Carry flag set edildiyse hata vardır

    pop dx
    cmp dh, al      ; Okunan sektör sayısı beklenenle aynı mı?
    jne disk_error
    
    popa
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string
    jmp $

DISK_ERROR_MSG db "Disk okuma hatasi!", 0
