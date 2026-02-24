; GümüşOS IDT (Interrupt Descriptor Table) Rugged Test Suite
; "Dayı Tavsiyesi" ile 32-bit Protected Mode ve tam IDT yapısına geçiş.

[org 0x7c00]
[bits 16]

; --- BAŞLANGIÇ: Donanım ve Yığın Hazırlığı ---
start:
    cli                         ; 1. Kural: Kesmeleri durdur (Dayı Tavsiyesi - Stack tehlikesi)

    xor ax, ax                  ; Segment koordinasyonu
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Yığını kodun altına güvenli bölgeye mühürle

    ; --- 32-BIT GEÇİŞ BİLETİ (GDT) ---
    lgdt [gdt_descriptor]

    ; Protected Mode'u aktifleştir (CR0 PE bit)
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; Uzun atlama (Far Jump) ile segmentleri tazele ve 32-bit dünyasına gir
    jmp 0x08:init_pm_32

[bits 32]
init_pm_32:
    ; 2. Kural: 32-bit Data Segmentlerini ayarla
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000            ; Saf ve yüksek bir stack alanı

    ; Ekranı temizle (32-bit Direct VGA Access)
    call clear_vga_32

    ; Sayfa Başlığı
    mov esi, MSG_DNA_START
    mov edi, 0xB8000 + 160 * 1 + 20 ; Line 1, Col 10
    mov ah, 0x0B                ; Light Cyan
    call print_vga_32

    ; --- TEST 1: IDT KURULUMU (Tapu Kaydı) ---
    mov esi, MSG_TEST_IDT
    mov edi, 0xB8000 + 160 * 3 + 2
    mov ah, 0x07                ; Grey
    call print_vga_32

    call setup_full_idt
    lidt [idt_descriptor_pm]    ; IDT'yi işlemciye tanıt

    mov esi, MSG_PASS
    mov edi, 0xB8000 + 160 * 3 + 60
    mov ah, 0x0A                ; Green
    call print_vga_32

    ; --- TEST 2: INTERRUPT TETİKLEME (int 0x80) ---
    mov esi, MSG_TEST_INT
    mov edi, 0xB8000 + 160 * 4 + 2
    mov ah, 0x07
    call print_vga_32

    ; INT 0x80 çağırılmadan önce handler'ın yazacağı yeri temizle
    mov dword [0xB8000 + 160 * 4 + 40], 0

    int 0x80                    ; Kesmeyi ateşle!

    ; Handler başarılı çalıştıysa oraya 'INT OK' yazmış olmalı
    cmp dword [0xB8000 + 160 * 4 + 44], 0x0E4B0E4F ; 'O', 'K' (Yellow)
    je .int_pass

    mov esi, MSG_FAIL
    mov edi, 0xB8000 + 160 * 4 + 60
    mov ah, 0x0C                ; Red
    call print_vga_32
    jmp .done

.int_pass:
    mov esi, MSG_PASS
    mov edi, 0xB8000 + 160 * 4 + 60
    mov ah, 0x0A
    call print_vga_32

.done:
    mov esi, MSG_FINISH
    mov edi, 0xB8000 + 160 * 6 + 2
    mov ah, 0x0B
    call print_vga_32

    jmp $                       ; Sonu...

; --- FONKSİYONLAR ---

setup_full_idt:
    ; Dayı Tavsiyesi: 256 girişin tamamını temizle
    ; (times 256 dq 0 ile zaten sıfırlandı ama setup_idt ile kapıları çalalım)
    
    ; int 0x80 kapısını kur (128. giriş -> 128 * 8 = 1024 offset)
    mov eax, test_handler_32
    mov [idt_pm + 1024], ax             ; Offset low
    mov word [idt_pm + 1024 + 2], 0x08   ; Selector (Kernel Code)
    mov byte [idt_pm + 1024 + 4], 0     ; Reserved
    mov byte [idt_pm + 1024 + 5], 0xEE  ; Access (Present, Ring 3, Int Gate)
    shr eax, 16
    mov [idt_pm + 1024 + 6], ax         ; Offset high
    ret

test_handler_32:
    ; Kesme geldiğinde ekrana "INT OK" bas
    mov dword [0xB8000 + 160 * 4 + 40], 0x0E4E0E49 ; 'I', 'N' (Sarı)
    mov dword [0xB8000 + 160 * 4 + 44], 0x0E4B0E4F ; 'O', 'K' (Sarı)
    iretd                               ; 32-bit Interrupt Return (Dayı Tavsiyesi 3)

clear_vga_32:
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0F20              ; Beyaz boşluk
    rep stosw
    ret

; ESI = String, EDI = VGA Addr, AH = Color
print_vga_32:
.loop:
    lodsb
    or al, al
    jz .done
    mov [edi], al
    mov [edi + 1], ah
    add edi, 2
    jmp .loop
.done:
    ret

; --- VERİ YAPILARI (Hizalı ve Zırhlı) ---

align 16
gdt_start:
    dq 0x0                      ; Bos (Null)
gdt_code:
    dq 0x00CF9A000000FFFF       ; Code Segment (Ring 0)
gdt_data:
    dq 0x00CF92000000FFFF       ; Data Segment (Ring 0)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

align 16
idt_pm:
    times 256 dq 0              ; 256 x 8-byte giriş (Dayı Tavsiyesi - Full Tablo)

idt_descriptor_pm:
    dw 256 * 8 - 1              ; Boyut
    dd idt_pm                   ; Lineer Adres (Protected Mode Standardı)

; Mesajlar
MSG_DNA_START db "--- GUMUS OS IDT RIGOROUS TEST ---", 0
MSG_TEST_IDT  db "Layer 5: IDT Tapu Kaydi (Full 256)...", 0
MSG_TEST_INT  db "Layer 5: Interrupt Fire (int 0x80)...", 0
MSG_PASS      db "[PASS]", 0
MSG_FAIL      db "[FAIL]", 0
MSG_FINISH    db "Status: System is stable in Protected Mode.", 0

; Boot Signature
times 510-($-$$) db 0
dw 0xAA55
