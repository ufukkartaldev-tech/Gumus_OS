; Kernel'a giriş için Assembly köprüsü
[bits 32]

; 32-bit modda C kernel'ını çağır (Windows GCC mangling)
extern _kernel_main
call _kernel_main

jmp $            ; Eğer kernel'dan dönerse sonsuz döngüye gir
