; GümüşOS - "Uyanış" Bootloader
[org 0x7c00]
KERNEL_OFFSET equ 0x1000 ; Kernel'ı bu adrese yükleyeceğiz

mov [BOOT_DRIVE], dl

; VBE setup geçici olarak devre dışı - VGA text mode kullanacağız
; mov ax, 0x4f02
; mov bx, 0x4103 ; mode 0x103 | 0x4000 (LFB)
; int 0x10

; Query mode info to get LFB address - devre dışı
; mov ax, 0x4f01
; mov cx, 0x103
; mov di, 0x9000 ; buffer to store mode info
; int 0x10

; LFB address is at offset 40 (0x28) in the mode info block - devre dışı
; mov eax, [0x9028]
; mov [0x500], eax ; Kernel bu adresten okuyacak

; Yığın (Stack) kurulumu
mov bp, 0x9000
mov sp, bp

mov bx, MSG_REAL_MODE
call print_string

mov bx, MSG_LOADING
call print_string

mov bx, KERNEL_OFFSET ; Kernel'ın yükleneceği adres
call load_kernel      ; Diski oku

; BIOS sonucunu kontrol et
jc bios_error        ; BIOS hatası varsa

mov bx, MSG_LOADED
call print_string

; Kernel çağrısından önce test
mov word [0xB8000], 0x0F4B00  ; 'K' karakteri + beyaz renk

; 16-bit modda kernel'i çağır
call KERNEL_OFFSET      ; Kernel'a zıpla!
jmp $

kernel_error:
    mov bx, MSG_ERROR
    call print_string
    jmp $

disk_count_error:
    mov bx, MSG_COUNT_ERROR
    call print_string
    jmp $

bios_error:
    mov bx, MSG_BIOS_ERROR
    call print_string
    jmp $

jmp $ ; Asla buraya ulaşmamalı

%include "src/boot/print_16.asm"
%include "src/boot/disk.asm"
%include "src/boot/gdt.asm"

[bits 16]
switch_to_pm:
    cli                     ; Kesmeleri durdur
    lgdt [gdt_descriptor]   ; GDT'yi yükle
    
    mov eax, cr0
    or eax, 0x1             ; PE bitini ayarla
    mov cr0, eax
    
    ; 32-bit'e geçiş için far jump
    jmp CODE_SEG:init_pm

[bits 32]
init_pm:
    ; 32-bit'e geçtiğimizi hemen test et
    mov dword [0xB8000], 0x0F423200  ; '2' karakteri (0x32) + beyaz renk (0x0F)
    
    mov ax, DATA_SEG        ; Segment register'larını yeni GDT'ye göre ayarla
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; Yığını 32-bit'e uyarla
    mov esp, ebp

    call BEGIN_PM

BEGIN_PM:
    ; Kernel'dan önce bir test daha
    mov dword [0xB8004], 0x0F4B4500  ; 'E' karakteri (0x45) + beyaz renk (0x0F)
    
    call KERNEL_OFFSET      ; Kernel'a zıpla!
    jmp $

; Veriler
BOOT_DRIVE db 0
VBE_LFB dd 0
MSG_REAL_MODE db "16-bit Real Mode aktif. ", 0
MSG_LOADING db "Kernel yukleniyor... ", 0
MSG_LOADED db "Kernel yuklendi! ", 0
MSG_ERROR db "Kernel yukleme hatasi! ", 0
MSG_COUNT_ERROR db "Disk sayim hatasi! ", 0
MSG_BIOS_ERROR db "BIOS disk hatasi! ", 0
MSG_PROT_MODE db "32-bit Protected Mode basariyla aktif edildi.", 0

; 32-bit print fonksiyonu (VGA buffer kullanarak)
[bits 32]
print_string_pm:
    pusha
    mov edx, 0xb8000        ; VGA Video Belleği
.loop:
    mov al, [ebx]
    mov ah, 0x0f            ; Beyaz karakter, siyah arka plan
    cmp al, 0
    je .done
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop
.done:
    popa
    ret

times 510-($-$$) db 0
dw 0xaa55
