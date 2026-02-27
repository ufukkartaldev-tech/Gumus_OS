param([switch]$Minimal, [int]$Stage = 1)
# GümüşOS Derleme ve Çalıştırma Betiği (Uyanış v2.0)
# Bu betik tüm çekirdek ve sürücü dosyalarını derler.

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

# 1. Bootloader'ı derle
Write-Host "[1/5] Bootloader derleniyor..."
& $NASM -f bin src/boot/boot.asm -o boot.bin

# 2. Kernel Entry ve Çekirdek Köprüsü
Write-Host "[2/5] Kernel Entry ve Assembly modülleri derleniyor..."
if ($Minimal) {
  & $NASM -f win32 src/kernel/core/kernel_entry.asm -o kernel_entry.o
  & $NASM -f elf32 src/kernel/cpu/gdt_asm.asm -o gdt_asm_obj.o
} else {
  & $NASM -f elf32 src/kernel/core/kernel_entry.asm -o kernel_entry.o
  & $NASM -f elf32 src/kernel/cpu/interrupt.asm -o interrupt_asm.o
  & $NASM -f elf32 src/kernel/cpu/gdt_asm.asm -o gdt_asm_obj.o
}

# 3. Tüm C dosyalarını derle
Write-Host "[3/5] Çekirdek ve Sürücü C dosyaları derleniyor..."
if ($Minimal) {
  $inc = @(
    "-Isrc/kernel/core",
    "-Isrc/kernel/core/memory",
    "-Isrc/kernel/cpu"
  )
} else {
  $inc = @(
    "-Isrc/kernel/core",
    "-Isrc/kernel/core/memory",
    "-Isrc/kernel/cpu",
    "-Isrc/kernel/drivers",
    "-Isrc/kernel/drivers/graphics",
    "-Isrc/kernel/drivers/input",
    "-Isrc/kernel/drivers/storage",
    "-Isrc/kernel/drivers/audio",
    "-Isrc/kernel/fs",
    "-Isrc/kernel/drivers/usb/core",
    "-Isrc/kernel/apps",
    "-Isrc/kernel/apps/system"
  )
}
$gcc_flags = @("-m32","-ffreestanding","-fno-pie","-fno-stack-protector","-nostdlib")

if ($Minimal) {
  $dirs = @(
    "src/kernel/core",
    "src/kernel/core/memory",
    "src/kernel/cpu"
  )
} else {
  # Derlenecek dizinler (Tam Derleme)
  $dirs = @(
      "src/kernel/core",
      "src/kernel/core/memory",
      "src/kernel/cpu",
      "src/kernel/drivers",
      "src/kernel/drivers/graphics",
      "src/kernel/drivers/input",
      "src/kernel/drivers/storage",
      "src/kernel/fs",
      "src/kernel/drivers/usb/core",
      "src/kernel/drivers/usb/host/ehci",
      "src/kernel/drivers/usb/host/ohci",
      "src/kernel/drivers/usb/host/xhci",
      "src/kernel/drivers/usb/class/hid",
      "src/kernel/apps/system",
      "src/kernel/apps/multimedia",
      "src/kernel/apps/games"
  )
}

$obj_list = @("kernel_entry.o", "gdt_asm_obj.o")
if (-not $Minimal) { $obj_list = @("kernel_entry.o","interrupt_asm.o","gdt_asm_obj.o") }

foreach ($dir in $dirs) {
    $files = Get-ChildItem -Path $dir -Filter *.c
    foreach ($file in $files) {
        if ($Minimal) {
          $wl1 = @("simple_kernel.c","gdt.c","gdt_stub.c","string.c")
          $wl2 = $wl1 + @("stdio.c","printf.c","utf8.c","window.c")
          $wl2 += @() # core ekleri
          $wl2gfx = @("vga_gfx.c","vga_font.c")
          $whitelist = $wl1
          if ($Stage -ge 2) { $whitelist = $wl2 }
          if ($Stage -ge 2 -and $dir -like "src/kernel/drivers/graphics") {
            $whitelist = $wl2gfx
          }
          if (-not ($whitelist -contains $file.Name)) { continue }
        } else {
          if ($file.Name -match "test|mini|simple|tiny") { continue }
        }
        
        $obj_name = $file.Name.Replace(".c", ".o")
        Write-Host "  Compiling $($file.Name)..."
        & $GCC @gcc_flags @inc -c $file.FullName -o $obj_name
        $obj_list += $obj_name
    }
}

# 4. Kernel Linkleme
Write-Host "[4/5] Kernel bağlanıyor (Linking)..."
& $LD -m i386pe -Ttext 0x1000 --allow-multiple-definition -o kernel.tmp $obj_list
& $OBJCOPY -O binary kernel.tmp kernel.bin

# 5. OS Image oluştur (Bootloader + Kernel)
Write-Host "[5/5] OS-Image oluşturuluyor..."
$fullImage = New-Object byte[] (1440 * 1024) # 1.44 MB Floppy Size
[Array]::Clear($fullImage, 0, $fullImage.Length)

$bootBin = [System.IO.File]::ReadAllBytes("boot.bin")
$kernelBin = [System.IO.File]::ReadAllBytes("kernel.bin")

[Array]::Copy($bootBin, 0, $fullImage, 0, $bootBin.Length)
[Array]::Copy($kernelBin, 0, $fullImage, 512, [Math]::Min($kernelBin.Length, $fullImage.Length - 512))

[System.IO.File]::WriteAllBytes("gumus_os.bin", $fullImage)

Write-Host "Derleme Tamamlandı!" -ForegroundColor Green
Write-Host "QEMU başlatılıyor..."
if ($Minimal) {
  & $QEMU -drive format=raw,file=gumus_os.bin -serial stdio -no-reboot -no-shutdown
} else {
  & $QEMU -drive format=raw,file=gumus_os.bin -serial stdio -no-reboot -no-shutdown -audiodev dsound,id=snd0 -machine pcspk-audiodev=snd0
}
