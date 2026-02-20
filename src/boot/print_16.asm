; 16-bit Real Mode String Yazdırma
print_string:
    pusha
.loop:
    mov al, [bx]
    cmp al, 0
    je .done
    mov ah, 0x0e
    int 0x10
    add bx, 1
    jmp .loop
.done:
    popa
    ret

print_newline:
    pusha
    mov ah, 0x0e
    mov al, 0x0a ; \n
    int 0x10
    mov al, 0x0d ; \r
    int 0x10
    popa
    ret
