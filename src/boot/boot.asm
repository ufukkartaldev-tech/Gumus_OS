; GümüşOS - "Uyanış" Bootloader
[org 0x7c00]
KERNEL_OFFSET equ 0x1000 ; Kernel'ı bu adrese yükleyeceğiz

mov [BOOT_DRIVE], dl

; VBE setup (800x600 256 colors)
mov ax, 0x4f02
mov bx, 0x4103 ; mode 0x103 | 0x4000 (LFB)
int 0x10

; Query mode info to get LFB address
mov ax, 0x4f01
mov cx, 0x103
mov di, 0x9000 ; buffer to store mode info
int 0x10

; LFB address is at offset 40 (0x28) in the mode info block
mov eax, [0x9028]
mov [0x500], eax ; Kernel bu adresten okuyacak

; Yığın (Stack) kurulumu
mov bp, 0x9000
mov sp, bp

mov bx, MSG_REAL_MODE
call print_string

mov bx, KERNEL_OFFSET ; Kernel'ın yükleneceği adres
mov dh, 50            ; 50 sektör oku (yaklaşık 25 KB) - Kernel büyüdü!
call load_kernel      ; Diski oku
call switch_to_pm     ; 32-bit'e geç

jmp $ ; Asla buraya ulaşmamalı

%include "src/boot/print_16.asm"
%include "src/boot/disk.asm"
%include "src/boot/gdt.asm"

[bits 16]
switch_to_pm:
    cli                     ; Kesmeleri durdur
    lgdt [gdt_descriptor]   ; GDT'yi yükle
    mov eax, cr0
    or eax, 0x1             ; cr0 register'ının ilk bitini 1 yap (PE bit)
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; Far jump ile segment register'larını güncelle

[bits 32]
init_pm:
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
    ; mov ebx, MSG_PROT_MODE
    ; call print_string_pm ; Mode 13h'de text buffer (0xb8000) çalışmaz, kapattık.
    
    call KERNEL_OFFSET      ; Kernel'a zıpla!
    jmp $

; Veriler
BOOT_DRIVE db 0
VBE_LFB dd 0
MSG_REAL_MODE db "16-bit Real Mode aktif. Uyanis basliyor...", 0
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
