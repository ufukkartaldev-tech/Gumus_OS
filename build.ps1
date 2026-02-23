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
Write-Host "[2/5] Kernel Entry ve Kesme sarmalayıcıları derleniyor..."
& $NASM -f win32 -o kernel_entry.o src/kernel/core/kernel_entry.asm
& $NASM -f win32 -o gdt_asm.o src/kernel/cpu/gdt_asm.asm

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
    "-Isrc/kernel/drivers/network",
    "-Isrc/kernel/drivers/storage",
    "-Isrc/kernel/drivers/usb/core",
    "-Isrc/kernel/drivers/usb/host",
    "-Isrc/kernel/drivers/usb/class",
    "-Isrc/kernel/fs",
    "-Isrc/kernel/apps",
    "-Isrc/kernel/apps/games",
    "-Isrc/kernel/apps/multimedia",
    "-Isrc/kernel/apps/system",
    "-Isrc/kernel/apps/utilities"
)

$FILES = @(
    @{ src = "src/kernel/core/kernel.c"; obj = "kernel.o" },
    @{ src = "src/kernel/cpu/idt.c"; obj = "idt.o" },
    @{ src = "src/kernel/core/string.c"; obj = "string.o" },
    @{ src = "src/kernel/apps/utilities/gumus_dil.c"; obj = "gumus_dil.o" },
    @{ src = "src/kernel/drivers/storage/ata.c"; obj = "ata.o" },
    @{ src = "src/kernel/fs/fs.c"; obj = "fs.o" },
    @{ src = "src/kernel/drivers/graphics/vga_gfx.c"; obj = "vga_gfx.o" },
    @{ src = "src/kernel/drivers/graphics/vga_font.c"; obj = "vga_font.o" },
    @{ src = "src/kernel/drivers/storage/cmos.c"; obj = "cmos.o" },
    @{ src = "src/kernel/drivers/audio/sound.c"; obj = "sound.o" },
    @{ src = "src/kernel/drivers/audio/advanced_sound.c"; obj = "advanced_sound.o" },
    @{ src = "src/kernel/drivers/input/mouse.c"; obj = "mouse.o" },
    @{ src = "src/kernel/apps/system/shell.c"; obj = "shell.o" },
    @{ src = "src/kernel/core/memory/memory.c"; obj = "memory.o" },
    @{ src = "src/kernel/cpu/task.c"; obj = "task.o" },
    @{ src = "src/kernel/core/window.c"; obj = "window.o" },
    @{ src = "src/kernel/apps/system/file_manager.c"; obj = "file_manager.o" },
    @{ src = "src/kernel/apps/system/file_manager_gui.c"; obj = "file_manager_gui.o" },
    @{ src = "src/kernel/apps/system/system_monitor.c"; obj = "system_monitor.o" },
    @{ src = "src/kernel/apps/multimedia/audio_mixer.c"; obj = "audio_mixer.o" },
    @{ src = "src/kernel/apps/games/snake.c"; obj = "snake.o" },
    @{ src = "src/kernel/apps/system/start_menu.c"; obj = "start_menu.o" },
    @{ src = "src/kernel/apps/utilities/calculator.c"; obj = "calculator.o" },
    @{ src = "src/kernel/apps/multimedia/paint.c"; obj = "paint.o" },
    @{ src = "src/kernel/apps/multimedia/image_viewer.c"; obj = "image_viewer.o" },
    @{ src = "src/kernel/drivers/driver_manager.c"; obj = "driver_manager.o" },
    @{ src = "src/kernel/fs/vfs.c"; obj = "vfs.o" },
    @{ src = "src/kernel/core/elf_loader.c"; obj = "elf_loader.o" },
    @{ src = "src/kernel/core/syscall.c"; obj = "syscall.o" },
    @{ src = "src/kernel/cpu/gdt.c"; obj = "gdt.o" },
    @{ src = "src/kernel/core/utf8.c"; obj = "utf8.o" },
    @{ src = "src/kernel/drivers/network/ethernet.c"; obj = "ethernet.o" },
    @{ src = "src/kernel/drivers/network/arp.c"; obj = "arp.o" },
    @{ src = "src/kernel/drivers/network/ip.c"; obj = "ip.o" },
    @{ src = "src/kernel/drivers/network/icmp.c"; obj = "icmp.o" },
    @{ src = "src/kernel/drivers/network/udp.c"; obj = "udp.o" },
    @{ src = "src/kernel/drivers/network/tcp.c"; obj = "tcp.o" },
    @{ src = "src/kernel/drivers/hardware_detect.c"; obj = "hardware_detect.o" },
    @{ src = "src/kernel/drivers/storage/ahci.c"; obj = "ahci.o" },
    @{ src = "src/kernel/drivers/graphics/vesa_vbe.c"; obj = "vesa_vbe.o" },
    @{ src = "src/kernel/drivers/network_driver.c"; obj = "network_driver.o" },
    @{ src = "src/kernel/drivers/audio/audio_driver.c"; obj = "audio_driver.o" },
    @{ src = "src/kernel/drivers/usb/core/usb_host.c"; obj = "usb_host.o" },
    @{ src = "src/kernel/drivers/serial.c"; obj = "serial.o" },
    @{ src = "src/kernel/drivers/pseudo.c"; obj = "pseudo.o" },
    @{ src = "src/kernel/core/stdio.c"; obj = "stdio.o" },
    @{ src = "src/kernel/core/stdlib.c"; obj = "stdlib.o" },
    @{ src = "src/kernel/core/math.c"; obj = "math.o" },
    @{ src = "src/kernel/core/globals.c"; obj = "globals.o" },
    @{ src = "src/kernel/core/interrupt_stubs.c"; obj = "interrupt_stubs.o" },
    @{ src = "src/kernel/drivers/graphics/font_8x8.c"; obj = "font_8x8.o" },
    @{ src = "src/kernel/drivers/usb/stub_drivers.c"; obj = "stub_drivers.o" },
    @{ src = "src/kernel/drivers/usb/hid_stub.c"; obj = "hid_stub.o" }
)

foreach ($file in $FILES) {
    Write-Host "Derleniyor: $($file.src)"
    & $GCC -ffreestanding -fno-common @INCLUDES -c $($file.src) -o $($file.obj)
}

# 4. Kernel'ı Linkle ve Binary'ye Çevir
Write-Host "[4/5] Kernel dosyaları bağlanıyor ve binary'ye çevriliyor..."
$OBJS = @("kernel_entry.o", "gdt_asm.o") + ($FILES | ForEach-Object { $_.obj })
& $LD -o kernel.tmp -Ttext 0x1000 --allow-multiple-definition @OBJS
& $OBJCOPY -O binary kernel.tmp kernel.bin

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
