[bits 32]
[extern _kernel_main]

_start:
    ; Kernel ana fonksiyonuna zıpla
    call _kernel_main
    
    ; Kernel dönerse (asla olmamalı) sonsuz döngü
    jmp $
