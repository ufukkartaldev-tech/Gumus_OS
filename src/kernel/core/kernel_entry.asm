; Kernel'a giriş için Assembly köprüsü
[bits 32]
[extern _kernel_main] ; C'deki fonksiyonu referans al (MinGW 32-bit'te _ prefix bulunur)

call _kernel_main ; C kernel'ını çalıştır
jmp $            ; Eğer kernel'dan dönerse sonsuz döngüye gir
