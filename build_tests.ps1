# GümüşOS Unit Test Build Script
# Tüm unit testleri derler ve çalıştırır

$ErrorActionPreference = "Continue"

# Araç yolları
$env:PATH = "C:\msys64\mingw32\bin;C:\Program Files\qemu;" + $env:PATH

$NASM = "nasm.exe"
$GCC = "gcc.exe"
$LD = "ld.exe"
$OBJCOPY = "objcopy.exe"

Write-Host "--- GümüşOS Unit Test Suite ---" -ForegroundColor Cyan

$inc = "-Isrc/kernel/core -Isrc/kernel/core/memory -Isrc/kernel/cpu -Isrc/kernel/drivers -Itests"

# Pre-compile core dependencies
Write-Host "Dependencies derleniyor..." -ForegroundColor Yellow
& $GCC -m32 -ffreestanding $inc -c src/kernel/core/string.c -o string.o
& $GCC -m32 -ffreestanding $inc -c src/kernel/core/math.c -o math.o
& $GCC -m32 -ffreestanding $inc -c src/kernel/core/memory/memory.c -o memory.o
& $GCC -m32 -ffreestanding $inc -c src/kernel/cpu/task.c -o task.o
& $GCC -m32 -ffreestanding $inc -c src/kernel/core/syscall.c -o syscall.o
& $GCC -m32 -ffreestanding $inc -c tests/mocks.c -o mocks.o

$all_deps = @("string.o", "math.o", "memory.o", "task.o", "syscall.o", "mocks.o")

# Test listesi
$tests = @(
    @{ name = "Kernel Core Test"; file = "tests/kernel_test.c"; output = "kernel_test.bin" },
    @{ name = "Memory Test"; file = "tests/memory_test.c"; output = "memory_test.bin" },
    @{ name = "Process Test"; file = "tests/process_test.c"; output = "process_test.bin" },
    @{ name = "Syscall Test"; file = "tests/syscall_test.c"; output = "syscall_test.bin" }
)

foreach ($t in $tests) {
    Write-Host "`nDerleniyor: $($t.name)" -ForegroundColor Yellow
    
    # 1. Test objesini derle
    & $GCC -m32 -ffreestanding $inc -c $t.file -o test_obj.o
    
    # 2. Wrapper
    $wrapper = @'
[bits 32]
[extern kernel_main]
global _start
_start:
  call kernel_main
  jmp $
'@
    $wrapper | Out-File -FilePath "test_wrapper.asm" -Encoding ASCII
    & $NASM -f elf32 test_wrapper.asm -o test_wrapper.o
    
    # 3. Link
    $objects = @("test_wrapper.o", "test_obj.o") + $all_deps
    & $LD -m i386pe -o test.tmp -Ttext 0x1000 --allow-multiple-definition $objects
    
    if ($LASTEXITCODE -eq 0) {
        # 4. Binary
        & $OBJCOPY -O binary test.tmp test.bin
        
        # 5. Bootloader image
        $boot_asm = @'
[org 0x7c00]
bits 16
mov ax, 0x1000
mov es, ax
mov bx, 0
mov ah, 0x02
mov al, 15
mov ch, 0
mov dh, 0
mov cl, 2
int 0x13
jmp 0x1000:0
times 510-($-$$) db 0
dw 0xAA55
'@
        $boot_asm | Out-File -FilePath "test_boot.asm" -Encoding ASCII
        & $NASM -f bin test_boot.asm -o test_boot.bin
        
        # 6. Combined image
        $b1 = [System.IO.File]::ReadAllBytes("test_boot.bin")
        $b2 = [System.IO.File]::ReadAllBytes("test.bin")
        $f = New-Object byte[] 8192
        [Array]::Copy($b1, 0, $f, 0, 512)
        [Array]::Copy($b2, 0, $f, 512, [Math]::Min($b2.Length, 7680))
        [System.IO.File]::WriteAllBytes($t.output, $f)
        
        Write-Host "✓ $($t.output) hazir" -ForegroundColor Green
    }
    else {
        Write-Host "✗ $($t.name) linkleme hatasi" -ForegroundColor Red
    }
}
