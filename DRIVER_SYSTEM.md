# GümüşOS Donanım Sürücü Sistemi

Bu doküman GümüşOS için geliştirilen kapsamlı donanım tespiti ve sürücü sistemini açıklamaktadır.

## Genel Bakış

GümüşOS donanım sürücü sistemi şu özellikleri içerir:

- **Gelişmiş PCI tespit ve konfigürasyon sistemi**
- **Universal disk sürücüsü (AHCI/SATA)**
- **USB HID sürücüleri (keyboard, mouse)**
- **Modern video sürücüsü (VESA/VBE)**
- **Network kartı sürücüleri (Realtek, Intel)**
- **Audio sürücüleri (AC'97, HDA)**
- **Sürücü yükleme ve kaldırma sistemi**

## Sistem Mimarisi

### 1. Sürücü Yöneticisi (`driver_manager.c`)

Sürücü yöneticisi tüm sürücüleri yönetir ve şu özellikleri sunar:

- Sürücü kayıt ve aktivasyon
- Dinamik sürücü yükleme/kaldırma
- Sürücü listeleme ve durum takibi
- Otomatik sürücü yükleme

**Ana Fonksiyonlar:**
```c
void driver_register(driver_t* driver);
int driver_activate(const char* name);
int driver_deactivate(const char* name);
int driver_unregister(const char* name);
void driver_list_all();
void driver_list_active();
int driver_auto_load_all();
```

### 2. Donanım Tespit Sistemi (`hardware_detect.c`)

PCI üzerinden donanım tespiti yapar ve uygun sürücüleri otomatik olarak yükler:

- PCI aygıtlarını tara (0-255 bus, 0-31 device, 0-7 function)
- Aygıt bilgilerini oku (vendor ID, device ID, class code)
- Aygıtları sınıflandır ve sürücü eşleştir
- ACPI tablolarını tespit et

**Desteklenen PCI Sınıfları:**
- Storage (IDE, SATA, NVMe)
- Display (VGA)
- Network (Ethernet)
- Multimedia (Audio)
- Serial (USB)

## Sürücü Detayları

### 1. AHCI/SATA Sürücüsü (`ahci.c`)

Modern SATA diskler için AHCI 1.3 standardını destekler:

**Özellikler:**
- NCQ (Native Command Queuing) desteği
- 32-bit/64-bit adresleme
- Hot-plug desteği
- Multi-port desteği

**Ana Fonksiyonlar:**
```c
int ahci_read(ahci_driver_t* driver, void* buffer, uint32_t lba, uint32_t count);
int ahci_write(ahci_driver_t* driver, void* buffer, uint32_t lba, uint32_t count);
int ahci_identify(ahci_driver_t* driver);
```

### 2. USB HID Sürücüleri (`usb_hid.c`)

USB HID aygıtları için tam destek:

**Desteklenen Aygıtlar:**
- USB Keyboard (HID boot protocol)
- USB Mouse (HID boot protocol)

**Özellikler:**
- Otomatik aygıt tanıma
- Scancode çevirme
- Multi-key desteği
- Mouse hareket ve buton takibi

### 3. VESA/VBE Video Sürücüsü (`vesa_vbe.c`)

Modern grafik kartları için VESA BIOS Extensions 3.0 desteği:

**Özellikler:**
- Linear framebuffer erişimi
- Çoklu çözünürlük desteği (640x480 - 1280x1024)
- 8/16/24/32-bit renk derinliği
- 2D grafik fonksiyonları

**Grafik Fonksiyonları:**
```c
int vesa_put_pixel(int x, int y, rgba_t color);
int vesa_fill_rect(int x, int y, int width, int height, rgba_t color);
int vesa_draw_line(int x1, int y1, int x2, int y2, rgba_t color);
int vesa_draw_string(int x, int y, const char* str, rgba_t fg_color, rgba_t bg_color);
```

### 4. Network Sürücüleri (`network_driver.c`)

Ethernet kartları için tam TCP/IP desteği:

**Desteklenen Kartlar:**
- Realtek RTL8139
- Intel E1000 serisi

**Özellikler:**
- Ethernet frame işleme
- ARP desteği
- ICMP (ping) desteği
- IP checksum hesaplama
- Network statistics

**Network Fonksiyonları:**
```c
int network_send_packet(network_driver_t* driver, uint8_t* data, uint32_t size);
int network_receive_packet(network_driver_t* driver, uint8_t* buffer, uint32_t* size);
int network_send_arp_request(network_driver_t* driver, uint32_t target_ip);
int network_send_ping(network_driver_t* driver, uint32_t target_ip);
```

### 5. Audio Sürücüleri (`audio_driver.c`)

Ses kartları için tam audio desteği:

**Desteklenen Standartlar:**
- AC'97 (Audio Codec '97)
- Intel HDA (High Definition Audio)

**Özellikler:**
- Çoklu sample rate (8K - 96K Hz)
- 8/16/24/32-bit audio
- Mono/Stereo/5.1/7.1 kanal desteği
- Volume kontrol ve mute
- Playback ve capture

**Audio Fonksiyonları:**
```c
int audio_set_format(audio_driver_t* driver, audio_format_t* format);
int audio_start_playback(audio_driver_t* driver);
int audio_write_samples(audio_driver_t* driver, void* samples, uint32_t count);
int audio_set_volume(audio_driver_t* driver, uint16_t volume);
```

## Kullanım Örnekleri

### Donanım Tespiti
```c
// Donanım tespit sistemini başlat
hardware_detect_init();

// Tespit edilen donanımı özetle
hardware_print_summary();

// Otomatik sürücü yükleme
driver_auto_load_all();
```

### Sürücü Yönetimi
```c
// Tüm sürücüleri listele
driver_list_all();

// Aktif sürücüleri listele
driver_list_active();

// Sürücü aktifleştir
driver_activate("AHCI SATA");

// Sürücü devre dışı bırak
driver_deactivate("AHCI SATA");
```

### Video Sürücüsü
```c
// VESA sürücüsünü başlat
driver_t* vesa = create_vesa_driver(NULL);
driver_register(vesa);
driver_activate("VESA/VBE Graphics");

// Ekranı temizle
rgba_t black = {0, 0, 0, 255};
vesa_clear_screen(black);

// Çizim yap
rgba_t white = {255, 255, 255, 255};
vesa_draw_string(10, 10, "Merhaba GümüşOS!", white, black);
```

### Network Sürücüsü
```c
// Network sürücüsünü başlat
driver_t* net = create_rtl8139_driver(NULL);
driver_register(net);
driver_activate("RTL8139 Ethernet");

// IP konfigürasyonu
uint32_t ip = network_parse_ip("192.168.1.100");
uint32_t gateway = network_parse_ip("192.168.1.1");
network_set_ip_config(&rtl8139_driver.base, ip, 0xFFFFFF00, gateway);

// Ping gönder
network_send_ping(&rtl8139_driver.base, network_parse_ip("8.8.8.8"));
```

## Dosya Yapısı

```
src/kernel/drivers/
├── driver.h              # Sürücü arayüzü
├── driver_manager.c      # Sürücü yöneticisi
├── hardware_detect.h     # Donanım tespiti arayüzü
├── hardware_detect.c    # Donanım tespiti implementasyonu
├── ahci.h               # AHCI sürücüsü arayüzü
├── ahci.c               # AHCI sürücüsü implementasyonu
├── usb_hid.h            # USB HID arayüzü
├── usb_hid.c            # USB HID implementasyonu
├── vesa_vbe.h           # VESA/VBE arayüzü
├── vesa_vbe.c           # VESA/VBE implementasyonu
├── network_driver.h     # Network sürücüleri arayüzü
├── network_driver.c     # Network sürücüleri implementasyonu
├── audio_driver.h       # Audio sürücüleri arayüzü
└── audio_driver.c       # Audio sürücüleri implementasyonu
```

## Desteklenen Donanım

### Storage
- IDE/ATA
- SATA/AHCI
- NVMe (planlı)

### Video
- VGA
- VESA/VBE uyumlu kartlar

### Network
- Realtek RTL8139/8169
- Intel E1000/PRO/1000
- Broadcom (planlı)

### Audio
- AC'97 codec'ler
- Intel HDA codec'ler
- Realtek ALC serisi

### Input
- PS/2 Keyboard/Mouse
- USB HID Keyboard/Mouse

## Gelecek Planları

1. **NVMe Desteği** - Modern SSD'ler için NVMe sürücüsü
2. **WiFi Sürücüleri** - Kablosuz ağ kartları için destek
3. **Bluetooth** - Bluetooth aygıtları için sürücü
4. **USB 3.0** - Yüksek hızlı USB desteği
5. **GPU Sürücüleri** - Modern GPU'lar için 3D destek

## Notlar

- Tüm sürücüler modüler olarak tasarlanmıştır
- Hot-plug desteği planlanmaktadır
- Driver signing ve güvenlik özellikleri eklenecektir
- Power management desteği geliştirilecektir

Bu sistem GümüşOS'un modern donanımlarla tam uyumlu çalışmasını sağlamak için tasarlanmıştır.
