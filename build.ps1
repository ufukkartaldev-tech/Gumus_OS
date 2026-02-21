# GümüşOS Derleme ve Çalıştırma Betiği
# Bu betik MSYS2 ve QEMU'nun belirtilen yollarda kurulu olduğunu varsayar.

$ErrorActionPreference = "Stop"

# Araç Yolları ve Ortam Değişkenleri
$env:PATH = "C:\msys64\mingw32\bin;C:\Program Files\qemu;" + $env:PATH

$NASM = "nasm.exe"
$GCC = "gcc.exe"
$LD = "ld.exe"
$OBJCOPY = "objcopy.exe"
$QEMU = "qemu-system-x86_64.exe"

Write-Host "--- GümüşOS: 'Uyanış' Derleme Süreci Başladı ---" -ForegroundColor Cyan

# Araç kontrolü
if (-not (Get-Command $NASM -ErrorAction SilentlyContinue)) { throw "NASM bulunamadı. Lütfen MSYS2 kurulumunu kontrol edin." }
if (-not (Get-Command $GCC -ErrorAction SilentlyContinue)) { throw "GCC bulunamadı." }
if (-not (Get-Command $OBJCOPY -ErrorAction SilentlyContinue)) { throw "objcopy bulunamadı." }
if (-not (Get-Command $QEMU -ErrorAction SilentlyContinue)) { throw "QEMU bulunamadı." }

# 1. Bootloader'ı derle
Write-Host "[1/5] Bootloader derleniyor..."
& $NASM -f bin src/boot/boot.asm -o boot.bin

# 2. Kernel Entry ve Interrupts derleniyor
Write-Host "[2/5] Kernel Entry ve Kesme sarmalayicilari derleniyor..."
& $NASM -f elf32 src/kernel/core/kernel_entry.asm -o kernel_entry.o
& $NASM -f elf32 src/kernel/cpu/interrupt.asm -o interrupt.o
& $NASM -f elf32 src/kernel/cpu/gdt_asm.asm -o gdt_asm.o

# 3. Kernel Derleme (C Kodları)
Write-Host "[3/5] Kernel derleniyor..."
$INCLUDES = @("-Isrc/kernel/core", "-Isrc/kernel/drivers", "-Isrc/kernel/drivers/network", "-Isrc/kernel/fs", "-Isrc/kernel/cpu", "-Isrc/kernel/apps")

& $GCC -ffreestanding $INCLUDES -c src/kernel/core/kernel.c -o kernel.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/cpu/idt.c -o idt.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/string.c -o string.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/gumus_dil.c -o gumus_dil.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/ata.c -o ata.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/fs/fs.c -o fs.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/vga_gfx.c -o vga_gfx.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/vga_font.c -o vga_font.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/cmos.c -o cmos.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/sound.c -o sound.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/advanced_sound.c -o advanced_sound.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/mouse.c -o mouse.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/shell.c -o shell.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/memory.c -o memory.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/cpu/task.c -o task.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/window.c -o window.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/file_manager.c -o file_manager.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/file_manager_gui.c -o file_manager_gui.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/system_monitor.c -o system_monitor.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/audio_mixer.c -o audio_mixer.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/snake.c -o snake.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/start_menu.c -o start_menu.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/calculator.c -o calculator.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/paint.c -o paint.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/apps/image_viewer.c -o image_viewer.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/driver_manager.c -o driver_manager.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/fs/vfs.c -o vfs.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/elf_loader.c -o elf_loader.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/syscall.c -o syscall.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/cpu/gdt.c -o gdt.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/core/utf8.c -o utf8.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/ethernet.c -o ethernet.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/arp.c -o arp.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/ip.c -o ip.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/icmp.c -o icmp.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/udp.c -o udp.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network/tcp.c -o tcp.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/hardware_detect.c -o hardware_detect.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/ahci.c -o ahci.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/vesa_vbe.c -o vesa_vbe.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/network_driver.c -o network_driver.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/audio_driver.c -o audio_driver.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/usb_host.c -o usb_host.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/ohci_controller.c -o ohci_controller.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/ehci_xhci.c -o ehci_xhci.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/serial.c -o serial.o
& $GCC -ffreestanding $INCLUDES -c src/kernel/drivers/pseudo.c -o pseudo.o

# 4. Kernel'ı Linkle ve Binary'ye Çevir
Write-Host "[4/5] Kernel dosyaları bağlanıyor ve binary'ye çevriliyor..."
# 0x1000 adresine yerleştiriyoruz
# Not: kernel_entry.o en başta olmalı
& $LD -o kernel.tmp -Ttext 0x1000 kernel_entry.o interrupt.o kernel.o idt.o utf8.o vga_font.o memory.o shell.o string.o gumus_dil.o ata.o fs.o vga_gfx.o cmos.o sound.o advanced_sound.o mouse.o task.o window.o file_manager.o file_manager_gui.o system_monitor.o audio_mixer.o snake.o start_menu.o calculator.o paint.o image_viewer.o driver_manager.o vfs.o syscall.o gdt.o gdt_asm.o ethernet.o arp.o ip.o icmp.o udp.o tcp.o hardware_detect.o ahci.o vesa_vbe.o network_driver.o audio_driver.o usb_host.o ohci_controller.o ehci_xhci.o serial.o pseudo.o elf_loader.o
& $OBJCOPY -O binary kernel.tmp kernel.bin

# 5. OS Image oluştur (Bootloader + Kernel)
Write-Host "[5/5] OS-Image oluşturuluyor..."
$bootBin = [System.IO.File]::ReadAllBytes("boot.bin")
$kernelBin = [System.IO.File]::ReadAllBytes("kernel.bin")

# 512 byte'lık bootloader + kernel (en az 15 sektör okuyoruz boot.asm'de, o yüzden kernel'ı da pedleyelim)
# Not: Load kernel 15 sektör bekliyor olabilir, kontrol edelim.
$fullImage = New-Object byte[] (1440 * 1024) # 1.44 MB Floppy Size
[Array]::Clear($fullImage, 0, $fullImage.Length) # Sıfırla

[Array]::Copy($bootBin, 0, $fullImage, 0, $bootBin.Length)
[Array]::Copy($kernelBin, 0, $fullImage, 512, $kernelBin.Length)

# 6. Uygulama Paketleme (ELF Injection)
Write-Host "[6/6] Uygulamalar paketleniyor..."
$INCLUDES_UI = "-Isrc/user/lib"
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/lib/crt0.c -o crt0.o
& $GCC -m32 -ffreestanding $INCLUDES_UI -c src/user/hello/hello.c -o hello.o
# crt0.o en başta olmalı ve giriş noktası _start olmalı
& $LD -m i386pe -Ttext 0x400000 -e __start crt0.o hello.o -o hello.tmp
# Not: MinGW ld bazen _start yerine __start (çift alt tire) bekleyebilir. 
# Symbol kontrolü için objcopy elf çevrimi sonrası tam oturacaktır.
# Not: i386pe (MinGW) yerine elf_i386 deniyoruz, eğer hata alırsak binary'ye çevirip öyle yüklenebilir.
# Ancak ELF Loader gerçek ELF bekliyor.
& $OBJCOPY -O elf32-i386 hello.tmp hello.elf

if (Test-Path "hello.elf") {
    $helloElf = [System.IO.File]::ReadAllBytes("hello.elf")
    
    # Root Dir (Sektör 100 -> Offset 51200)
    # Basit bir FAT girişi hazırlıyoruz: 8 bayt isim, 3 bayt uzantı...
    $entry = New-Object byte[] 32
    $name = [System.Text.Encoding]::ASCII.GetBytes("HELLO   ")
    $ext = [System.Text.Encoding]::ASCII.GetBytes("ELF")
    [Array]::Copy($name, 0, $entry, 0, 8)
    [Array]::Copy($ext, 0, $entry, 8, 3)
    $entry[11] = 0x20 # Archive
    $entry[26] = 0x01 # First Cluster (Sektör 201)
    $entry[28] = ($helloElf.Length -band 0xFF)
    $entry[29] = ($helloElf.Length -shr 8 -band 0xFF)
    $entry[30] = ($helloElf.Length -shr 16 -band 0xFF)
    $entry[31] = ($helloElf.Length -shr 24 -band 0xFF)
    
    # 51200 adresine yaz
    [Array]::Copy($entry, 0, $fullImage, 51200, 32)
    # Veriyi 102912 adresine yaz (201 * 512)
    [Array]::Copy($helloElf, 0, $fullImage, 102912, $helloElf.Length)
}

[System.IO.File]::WriteAllBytes("gumus_os.bin", $fullImage)

Write-Host "`n--- Derleme Tamamlandı! ---" -ForegroundColor Green
Write-Host "QEMU başlatılıyor..."
& $QEMU -drive format=raw,file=gumus_os.bin -serial stdio -no-reboot -no-shutdown -audiodev dsound,id=snd0 -machine pcspk-audiodev=snd0
