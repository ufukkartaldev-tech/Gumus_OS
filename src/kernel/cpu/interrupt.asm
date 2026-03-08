; Assembly wrappers for interrupts
global isr0
global irq0
global irq1
global irq12
global isr128

extern isr_handler
extern irq_handler
extern syscall_handler

; İstisna (Divide by Zero vs)
isr0:
    cli
    push byte 0 ; Hata kodu yok
    push byte 0 ; İstisna Numarası
    jmp isr_common_stub

; IRQ 0 - Timer
irq0:
    cli
    push byte 0
    push byte 32
    jmp irq_common_stub

; IRQ 1 - Keyboard
irq1:
    cli
    push byte 0
    push byte 33
    jmp irq_common_stub

; IRQ 12 - Mouse
irq12:
    cli
    push byte 0
    push byte 44
    jmp irq_common_stub
    
; Syscall (int 0x80)
isr128:
    cli
    push byte 0
    push 0x80
    pusha           ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    
    mov ax, ds      ; Mevcut DS'i sakla
    push eax
    
    ; Kernel Segmentlerini Yükle
    mov ax, 0x10    ; Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp        ; registers_t*
    call syscall_handler
    mov esp, eax    ; YENI: Görev değişmiş olabilir (ör: SYS_EXIT), ESP'yi güncelle!
    
    pop eax         ; DS'i geri al
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popa            ; Restore registers (EAX changed in stack will be restored here)
    add esp, 8      ; Hata kodu ve int nosunu at
    sti
    iret

; Ortak Stub'lar
isr_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call isr_handler
    add esp, 4
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    sti
    iret

irq_common_stub:
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; registers* pointer'ı olarak ESP'yi gönder
    call irq_handler
    mov esp, eax        ; Yeni stack pointer'ı (görev değişmiş olabilir)
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    sti
    iret
