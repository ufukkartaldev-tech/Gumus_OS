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
& $LD -o kernel.tmp -Ttext 0x1000 kernel_entry.o interrupt.o kernel.o idt.o utf8.o vga_font.o memory.o shell.o string.o gumus_dil.o ata.o fs.o vga_gfx.o cmos.o sound.o advanced_sound.o mouse.o task.o window.o file_manager.o file_manager_gui.o system_monitor.o audio_mixer.o snake.o start_menu.o calculator.o paint.o image_viewer.o driver_manager.o vfs.o syscall.o gdt.o gdt_asm.o ethernet.o arp.o ip.o icmp.o hardware_detect.o ahci.o vesa_vbe.o network_driver.o audio_driver.o usb_host.o ohci_controller.o ehci_xhci.o serial.o pseudo.o elf_loader.o
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
# Kernel bootloader'dan hemen sonra, yani 512. byte'tan başlar
[Array]::Copy($kernelBin, 0, $fullImage, 512, $kernelBin.Length)

[System.IO.File]::WriteAllBytes("gumus_os.bin", $fullImage)

Write-Host "`n--- Derleme Tamamlandı! ---" -ForegroundColor Green
Write-Host "QEMU başlatılıyor..."
& $QEMU -drive format=raw,file=gumus_os.bin -serial stdio -no-reboot -no-shutdown -audiodev dsound,id=snd0 -machine pcspk-audiodev=snd0
