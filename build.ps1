# GümüşOS Derleme ve Çalıştırma Betiği
# Bu betik MSYS2 ve QEMU'nun belirtilen yollarda kurulu olduğunu varsayar.

$ErrorActionPreference = "Stop"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

# Araç Yolları ve Ortam Değişkenleri
$env:PATH = "C:\msys64\mingw32\bin;C:\Program Files\qemu;" + $env:PATH

$NASM = "nasm.exe"
$GCC = "gcc.exe"
$LD = "ld.exe"
$OBJCOPY = "objcopy.exe"
$QEMU = "qemu-system-x86_64.exe"

Write-Host "--- GümüşOS: 'Uyanış' Derleme Süreci Başladı ---" -ForegroundColor Cyan

# Araç kontrolü
if (-not (Get-Command $NASM -ErrorAction SilentlyContinue)) { throw "NASM bulunamadı." }
if (-not (Get-Command $GCC -ErrorAction SilentlyContinue)) { throw "GCC bulunamadı." }
if (-not (Get-Command $OBJCOPY -ErrorAction SilentlyContinue)) { throw "objcopy bulunamadı." }
if (-not (Get-Command $QEMU -ErrorAction SilentlyContinue)) { throw "QEMU bulunamadı." }

# 1. Bootloader'ı derle
Write-Host "[1/5] Bootloader derleniyor..."
& $NASM -f bin src/boot/boot.asm -o boot.bin

# 2. Kernel Entry ve Kesme sarmalayıcıları derleniyor...
Write-Host "[2/5] Kernel Entry ve Test Kernel derleniyor..."
& $NASM -f elf32 src/kernel/core/kernel_entry.asm -o kernel_entry.o
& $NASM -f elf32 src/kernel/core/test_kernel.asm -o test_kernel.o

# 3. Kernel Derleme (C Kodları)
Write-Host "[3/5] Kernel derleniyor..."
$INCLUDES = @(
    "-Isrc/kernel/core",
    "-Isrc/kernel/core/memory",
    "-Isrc/kernel/cpu",
    "-Isrc/kernel/drivers",
    "-Isrc/kernel/drivers/audio",
    "-Isrc/kernel/drivers/graphics",
    "-Isrc/kernel/drivers/input",
    "-Isrc/kernel/drivers/storage",
    "-Isrc/kernel/drivers/usb/core",
    "-Isrc/kernel/drivers/usb/class",
    "-Isrc/kernel/fs",
    "-Isrc/kernel/apps",
    "-Isrc/kernel/apps/games",
    "-Isrc/kernel/apps/multimedia",
    "-Isrc/kernel/apps/system",
    "-Isrc/kernel/apps/utilities"
)

$FILES = @(
    @{ src = "src/kernel/core/minimal_kernel.c"; obj = "kernel.o" }
)

foreach ($file in $FILES) {
    Write-Host "Derleniyor: $($file.src)"
    & $GCC -ffreestanding -fno-common @INCLUDES -c $($file.src) -o $($file.obj)
}

# 4. Kernel'ı Linkle ve Binary'ye Çevir
Write-Host "[4/5] Kernel dosyaları bağlanıyor..."
$OBJS = @("kernel_entry.o", "kernel.o")
& $LD -o kernel.tmp -Ttext 0x1000 --allow-multiple-definition @OBJS
& $OBJCOPY -O binary kernel.tmp kernel.bin

# 4. Uygulama Paketleme
Write-Host "[4/4] Uygulamalar paketleniyor..."
$INCLUDES_UI = "-Isrc/user/lib"
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/lib/crt0.c -o crt0.o
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/hello/hello.c -o hello.o
& $LD -m i386pe -Ttext 0x400000 -e __start crt0.o hello.o -o hello.tmp
# ELF loader için ELF32 lazım
& $OBJCOPY -O elf32-i386 hello.tmp hello.elf

# 5. OS Image oluştur (Bootloader + Kernel)
Write-Host "[5/5] OS-Image oluşturuluyor..."
$fullImage = New-Object byte[] (1440 * 1024) # 1.44 MB Floppy Size
[Array]::Clear($fullImage, 0, $fullImage.Length)

$bootBin = [System.IO.File]::ReadAllBytes("boot.bin")
$kernelBin = [System.IO.File]::ReadAllBytes("kernel.bin")

[Array]::Copy($bootBin, 0, $fullImage, 0, $bootBin.Length)
[Array]::Copy($kernelBin, 0, $fullImage, 512, $kernelBin.Length)

# 6. Uygulama Paketleme
Write-Host "[6/6] Uygulamalar paketleniyor..."
$INCLUDES_UI = "-Isrc/user/lib"
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/lib/crt0.c -o crt0.o
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/hello/hello.c -o hello.o
& $LD -m i386pe -Ttext 0x400000 -e __start crt0.o hello.o -o hello.tmp
# ELF loader için ELF32 lazım
& $OBJCOPY -O elf32-i386 hello.tmp hello.elf

if (Test-Path "hello.elf") {
    $helloElf = [System.IO.File]::ReadAllBytes("hello.elf")
    # Root Dir (Sektör 100 -> Offset 51200)
    $entry = New-Object byte[] 32
    [Array]::Copy([System.Text.Encoding]::ASCII.GetBytes("HELLO   "), 0, $entry, 0, 8)
    [Array]::Copy([System.Text.Encoding]::ASCII.GetBytes("ELF"), 0, $entry, 8, 3)
    $entry[11] = 0x20
    $entry[26] = 0x01
    $entry[28] = ($helloElf.Length -band 0xFF)
    $entry[29] = ($helloElf.Length -shr 8 -band 0xFF)
    $entry[30] = ($helloElf.Length -shr 16 -band 0xFF)
    $entry[31] = ($helloElf.Length -shr 24 -band 0xFF)
    [Array]::Copy($entry, 0, $fullImage, 51200, 32)
    [Array]::Copy($helloElf, 0, $fullImage, 102912, $helloElf.Length)
}

[System.IO.File]::WriteAllBytes("gumus_os.bin", $fullImage)

Write-Host "Derleme Tamamlandı!" -ForegroundColor Green
Write-Host "QEMU başlatılıyor..."
& $QEMU -drive format=raw,file=gumus_os.bin -serial stdio -no-reboot -no-shutdown -audiodev dsound,id=snd0 -machine pcspk-audiodev=snd0
