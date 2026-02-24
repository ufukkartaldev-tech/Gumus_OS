[org 0x7c00]
bits 16 
mov ax, 0x1000 
mov es, ax 
mov bx, 0 
mov ah, 0x02 
mov al, 25 
mov ch, 0 
mov dh, 0 
mov cl, 2 
int 0x13 
jmp 0x1000:0 
times 510-($-$$) db 0 
dw 0xAA55 
