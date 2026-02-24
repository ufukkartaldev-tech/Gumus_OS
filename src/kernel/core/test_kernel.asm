; Sadece 32-bit'e geçiş testi - hiç C kodu yok
[bits 32]

; Kernel yerine basit assembly testi
mov dword [0xB8000], 0x0F414100  ; 'A' karakteri (0x41) + beyaz renk (0x0F)

jmp $ ; Sonsuz döngü
