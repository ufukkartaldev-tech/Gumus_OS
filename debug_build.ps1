$env:PATH = "C:\msys64\mingw32\bin;C:\Program Files\qemu;" + $env:PATH
$INCLUDES = @(
  "-Isrc/kernel/core",
  "-Isrc/kernel/core/memory",
  "-Isrc/kernel/drivers",
  "-Isrc/kernel/drivers/audio",
  "-Isrc/kernel/drivers/graphics",
  "-Isrc/kernel/drivers/input",
  "-Isrc/kernel/drivers/network",
  "-Isrc/kernel/drivers/storage",
  "-Isrc/kernel/drivers/usb",
  "-Isrc/kernel/drivers/usb/core",
  "-Isrc/kernel/fs",
  "-Isrc/kernel/cpu",
  "-Isrc/kernel/apps",
  "-Isrc/kernel/apps/system"
)
gcc -ffreestanding $INCLUDES -c src/kernel/core/kernel.c -o kernel.o 2>&1 | Out-File -Encoding utf8 compile_log.txt
