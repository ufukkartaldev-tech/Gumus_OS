; Kernel'a giriş için Assembly köprüsü
[bits 16]

; 16-bit modda basit test
mov ax, 0xB800
mov es, ax
mov word [es:0], 0x0F41  ; 'A' karakteri + beyaz renk

jmp $            ; Sonsuz döngü
