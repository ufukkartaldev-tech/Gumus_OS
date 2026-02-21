#include "hardware_detect.h"
#include "../core/io.h"
#include "../core/string.h"
#include "../core/memory.h"
#include "../drivers/ata.h"
#include "../drivers/vga_gfx.h"
#include "../drivers/ahci.h"
#include "../drivers/usb_hid.h"
#include "../drivers/vesa_vbe.h"
#include "../drivers/network_driver.h"
#include "../drivers/audio_driver.h"
#include "../drivers/usb_host.h"

// Global değişkenler
static hardware_info_t hw_info;
static int hardware_detect_initialized = 0;

// PCI Konfigürasyon Okuma Fonksiyonları
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    return (dword >> ((offset & 2) * 8)) & 0xFFFF;
}

uint8_t pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    return (dword >> ((offset & 3) * 8)) & 0xFF;
}

void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

void pci_write_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    dword &= ~(0xFFFF << ((offset & 2) * 8));
    dword |= (value << ((offset & 2) * 8));
    pci_write_config_dword(bus, device, function, offset & 0xFC, dword);
}

void pci_write_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value) {
    uint32_t dword = pci_read_config_dword(bus, device, function, offset & 0xFC);
    dword &= ~(0xFF << ((offset & 3) * 8));
    dword |= (value << ((offset & 3) * 8));
    pci_write_config_dword(bus, device, function, offset & 0xFC, dword);
}

void hardware_detect_init() {
    if (hardware_detect_initialized) return;
    
    memset(&hw_info, 0, sizeof(hardware_info_t));
    
    printf("Donanım tespiti başlatılıyor...\n");
    
    hardware_detect_scan_pci();
    hardware_detect_usb();
    hardware_detect_acpi();
    
    hardware_detect_initialized = 1;
    
    printf("Donanım tespiti tamamlandı: %d PCI aygıtı bulundu\n", hw_info.pci_device_count);
}

void hardware_detect_scan_pci() {
    printf("PCI aygıları taranıyor...\n");
    
    // Tüm PCI aygıtlarını tara (0-255 bus, 0-31 device, 0-7 function)
    for (int bus = 0; bus < 256 && hw_info.pci_device_count < 32; bus++) {
        for (int device = 0; device < 32 && hw_info.pci_device_count < 32; device++) {
            for (int function = 0; function < 8 && hw_info.pci_device_count < 32; function++) {
                uint16_t vendor_id = pci_read_config_word(bus, device, function, 0x00);
                uint16_t device_id = pci_read_config_word(bus, device, function, 0x02);
                
                // Geçersiz vendor ID'si varsa aygıt yok
                if (vendor_id == 0xFFFF || vendor_id == 0x0000) {
                    if (function == 0) {
                        break; // Bu device'da function yok
                    }
                    continue;
                }
                
                // Aygıt bilgilerini oku
                pci_device_t* pci_dev = &hw_info.pci_devices[hw_info.pci_device_count];
                
                pci_dev->vendor_id = vendor_id;
                pci_dev->device_id = device_id;
                pci_dev->command = pci_read_config_word(bus, device, function, 0x04);
                pci_dev->status = pci_read_config_word(bus, device, function, 0x06);
                pci_dev->revision_id = pci_read_config_byte(bus, device, function, 0x08);
                pci_dev->prog_if = pci_read_config_byte(bus, device, function, 0x09);
                pci_dev->subclass = pci_read_config_byte(bus, device, function, 0x0A);
                pci_dev->class_code = pci_read_config_byte(bus, device, function, 0x0B);
                pci_dev->header_type = pci_read_config_byte(bus, device, function, 0x0E);
                pci_dev->interrupt_line = pci_read_config_byte(bus, device, function, 0x3C);
                pci_dev->interrupt_pin = pci_read_config_byte(bus, device, function, 0x3D);
                
                // BAR'ları oku
                for (int i = 0; i < 6; i++) {
                    pci_dev->bar[i] = pci_read_config_dword(bus, device, function, 0x10 + i * 4);
                }
                
                // Aygıtı sınıflandır
                switch (pci_dev->class_code) {
                    case PCI_CLASS_STORAGE:
                        if (pci_dev->subclass == PCI_SUBCLASS_STORAGE_IDE) {
                            hw_info.has_ide = 1;
                            printf("IDE Controller bulundu: %04X:%04X\n", vendor_id, device_id);
                        } else if (pci_dev->subclass == PCI_SUBCLASS_STORAGE_SATA) {
                            hw_info.has_sata = 1;
                            printf("SATA Controller bulundu: %04X:%04X\n", vendor_id, device_id);
                        }
                        break;
                        
                    case PCI_CLASS_DISPLAY:
                        if (pci_dev->subclass == PCI_SUBCLASS_DISPLAY_VGA) {
                            hw_info.has_vga = 1;
                            printf("VGA Controller bulundu: %04X:%04X\n", vendor_id, device_id);
                        }
                        break;
                        
                    case PCI_CLASS_NETWORK:
                        if (pci_dev->subclass == PCI_SUBCLASS_NETWORK_ETHERNET) {
                            hw_info.has_network = 1;
                            printf("Ethernet Controller bulundu: %04X:%04X\n", vendor_id, device_id);
                        }
                        break;
                        
                    case PCI_CLASS_MULTIMEDIA:
                        if (pci_dev->subclass == PCI_SUBCLASS_MULTIMEDIA_AUDIO) {
                            hw_info.has_audio = 1;
                            printf("Audio Controller bulundu: %04X:%04X\n", vendor_id, device_id);
                        }
                        break;
                }
                
                // Aygıt için sürücü yükle
                hardware_load_driver_for_device(pci_dev);
                
                hw_info.pci_device_count++;
                
                // Multi-function device kontrolü (header_type'ın en üst biti)
                if (function == 0 && !(pci_dev->header_type & 0x80)) {
                    break; // Multi-function değil, diğer function'ları atla
                }
            }
        }
    }
}

void hardware_detect_usb() {
    // USB host controller'larını tespit et
    printf("USB host controller'ları taranıyor...\n");
    
    // PCI üzerinden USB controller'ları ara
    for (int i = 0; i < hw_info.pci_device_count; i++) {
        pci_device_t* dev = &hw_info.pci_devices[i];
        
        // USB Controller class code'ları
        if ((dev->class_code == 0x0C && (dev->subclass == 0x03 || dev->subclass == 0x00)) ||
            (dev->class_code == 0x01 && dev->subclass == 0x01)) { // USB as storage
            
            hw_info.has_usb = 1;
            printf("USB Host Controller bulundu: %04X:%04X\n", dev->vendor_id, dev->device_id);
            
            // USB sürücüsü yükle
            driver_t* usb_driver = create_usb_driver(dev);
            if (usb_driver) {
                hw_info.detected_drivers[hw_info.driver_count++] = usb_driver;
                driver_register(usb_driver);
            }
        }
    }
}

void hardware_detect_acpi() {
    // ACPI tablolarını tespit et
    printf("ACPI tabloları taranıyor...\n");
    
    // RSDP (Root System Description Pointer) ara
    // 1. EBDA (Extended BIOS Data Area) - 0x0000040E'de adres var
    uint16_t ebda_seg = *((uint16_t*)0x0000040E);
    uint32_t ebda_base = ebda_seg * 16;
    
    printf("EBDA adresi: 0x%08X\n", ebda_base);
    
    // EBDA'da RSDP ara (ilk 1KB)
    for (uint32_t i = 0; i < 1024; i += 16) {
        uint8_t* rsdp_candidate = (uint8_t*)(ebda_base + i);
        if (memcmp(rsdp_candidate, "RSD PTR ", 8) == 0) {
            // Checksum kontrolü yap
            uint8_t checksum = 0;
            for (int j = 0; j < 20; j++) { // ACPI 1.0 RSDP 20 byte
                checksum += rsdp_candidate[j];
            }
            
            if (checksum == 0) {
                printf("ACPI RSDP bulundu (EBDA): 0x%08X\n", ebda_base + i);
                return;
            } else {
                printf("RSDP candidate bulundu ama checksum hatalı: 0x%02X\n", checksum);
            }
        }
    }
    
    // 2. BIOS ROM alanında ara (0x000E0000 - 0x000FFFFF)
    uint8_t* rsdp_area = (uint8_t*)0x000E0000;
    for (int i = 0; i < 0x20000; i += 16) {
        if (memcmp(rsdp_area + i, "RSD PTR ", 8) == 0) {
            // Checksum kontrolü yap
            uint8_t checksum = 0;
            for (int j = 0; j < 20; j++) { // ACPI 1.0 RSDP 20 byte
                checksum += rsdp_area[i + j];
            }
            
            if (checksum == 0) {
                printf("ACPI RSDP bulundu (BIOS): 0x%08X\n", 0x000E0000 + i);
                
                // ACPI 2.0+ kontrolü
                uint8_t* rsdp = rsdp_area + i;
                uint32_t length = *((uint32_t*)(rsdp + 20));
                if (length >= 36) { // ACPI 2.0+ 36+ byte
                    checksum = 0;
                    for (int j = 0; j < length; j++) {
                        checksum += rsdp[j];
                    }
                    if (checksum == 0) {
                        printf("ACPI 2.0+ RSDP doğrulandı, uzunluk: %d\n", length);
                    }
                }
                return;
            } else {
                printf("RSDP candidate bulundu ama checksum hatalı: 0x%02X\n", checksum);
            }
        }
    }
    
    printf("ACPI RSDP bulunamadı\n");
}

int hardware_load_driver_for_device(pci_device_t* device) {
    if (!device) return -1;
    
    driver_t* driver = hardware_get_best_driver(device->vendor_id, device->device_id, 
                                                 device->class_code, device->subclass);
    if (!driver) return -1;
    
    // Sürücüyü başlat
    if (driver->init && driver->init() == 0) {
        hw_info.detected_drivers[hw_info.driver_count++] = driver;
        driver_register(driver);
        
        printf("Sürücü yüklendi: %s (%04X:%04X)\n", driver->name, 
               device->vendor_id, device->device_id);
        return 0;
    }
    
    return -1;
}

driver_t* hardware_get_best_driver(uint16_t vendor_id, uint16_t device_id, uint8_t class_code, uint8_t subclass) {
    // Sınıf ve alt sınıfa göre sürücü seç
    switch (class_code) {
        case PCI_CLASS_STORAGE:
            if (subclass == PCI_SUBCLASS_STORAGE_IDE) {
                // IDE/ATA controller
                // pci_device_t info can be passed here if create_ide_driver is updated
                return create_ide_driver(NULL);
            } else if (subclass == PCI_SUBCLASS_STORAGE_SATA) {
                return create_sata_driver(NULL); 
            }
            break;
            
        case PCI_CLASS_DISPLAY:
            if (subclass == PCI_SUBCLASS_DISPLAY_VGA) {
                return create_vga_driver(NULL);
            }
            break;
            
        case PCI_CLASS_NETWORK:
            if (subclass == PCI_SUBCLASS_NETWORK_ETHERNET) {
                return create_network_driver(NULL);
            }
            break;
            
        case PCI_CLASS_MULTIMEDIA:
            if (subclass == PCI_SUBCLASS_MULTIMEDIA_AUDIO) {
                return create_audio_driver(NULL);
            }
            break;
            
        case PCI_CLASS_SERIAL:
            if (subclass == PCI_SUBCLASS_SERIAL_USB) {
                // USB Host Controller tipine göre sürücü seç
                pci_device_t temp_device = {0};
                temp_device.vendor_id = vendor_id;
                temp_device.device_id = device_id;
                temp_device.class_code = class_code;
                temp_device.subclass = subclass;
                
                // Program Interface'e göre controller tipini belirle
                uint8_t prog_if = pci_read_config_byte(0, 0, 0, 0x09);
                temp_device.prog_if = prog_if;
                
                switch (prog_if) {
                    case 0x10: // OHCI
                        return create_ohci_driver(&temp_device);
                    case 0x20: // EHCI
                        return create_ehci_driver(&temp_device);
                    case 0x30: // XHCI
                        return create_xhci_driver(&temp_device);
                    default:
                        printf("Bilinmeyen USB Controller tipi: %02X\n", prog_if);
                        break;
                }
            }
            break;
    }
    
    return NULL;
}

hardware_info_t* hardware_get_info() {
    if (!hardware_detect_initialized) {
        hardware_detect_init();
    }
    
    return &hw_info;
}

void hardware_print_summary() {
    if (!hardware_detect_initialized) {
        printf("Donanım tespiti henüz yapılmadı.\n");
        return;
    }
    
    printf("\n=== Donanım Özeti ===\n");
    printf("PCI Aygıtları: %d\n", hw_info.pci_device_count);
    printf("Yüklenen Sürücüler: %d\n", hw_info.driver_count);
    printf("IDE Controller: %s\n", hw_info.has_ide ? "Var" : "Yok");
    printf("SATA Controller: %s\n", hw_info.has_sata ? "Var" : "Yok");
    printf("VGA Controller: %s\n", hw_info.has_vga ? "Var" : "Yok");
    printf("Network Controller: %s\n", hw_info.has_network ? "Var" : "Yok");
    printf("Audio Controller: %s\n", hw_info.has_audio ? "Var" : "Yok");
    printf("USB Controller: %s\n", hw_info.has_usb ? "Var" : "Yok");
    printf("===================\n\n");
}

// Universal Sürücü Oluşturma Fonksiyonları
driver_t* create_ide_driver(pci_device_t* device) {
    // IDE sürücüsü oluştur
    // Bu mevcut ATA sürücüsünü kullanabilir
    return NULL; // Şimdilik mevcut sürücüyü kullan
}

driver_t* create_sata_driver(pci_device_t* device) {
    // SATA/AHCI sürücüsü oluştur
    printf("SATA/AHCI sürücüsü oluşturuluyor...\n");
    return create_ahci_driver(device);
}

driver_t* create_vga_driver(pci_device_t* device) {
    // VGA/VBE sürücüsü oluştur
    printf("VGA/VBE sürücüsü oluşturuluyor...\n");
    return create_vesa_driver(device);
}

driver_t* create_network_driver(pci_device_t* device) {
    // Network kartı sürücüsü oluştur
    printf("Network kartı sürücüsü oluşturuluyor...\n");
    
    // Vendor ID'ye göre doğru sürücüyü seç
    switch (device->vendor_id) {
        case 0x10EC: // Realtek
            if (device->device_id == 0x8139) {
                return create_rtl8139_driver(device);
            }
            break;
        case 0x8086: // Intel
            if ((device->device_id & 0x1000) == 0x1000) {
                return create_e1000_driver(device);
            }
            break;
    }
    
    printf("Desteklenmeyen network kartı: %04X:%04X\n", device->vendor_id, device->device_id);
    return NULL;
}

driver_t* create_audio_driver(pci_device_t* device) {
    // Audio sürücüsü oluştur
    printf("Audio sürücüsü oluşturuluyor...\n");
    
    // Vendor ID'ye göre doğru sürücüyü seç
    switch (device->vendor_id) {
        case 0x8086: // Intel
            if (device->device_id >= 0x2668 && device->device_id <= 0x266A) {
                return create_hda_driver(device);
            }
            break;
        case 0x10DE: // NVIDIA
            if (device->device_id >= 0x003A && device->device_id <= 0x0059) {
                return create_hda_driver(device);
            }
            break;
        case 0x1002: // AMD/ATI
            if (device->device_id >= 0x4370 && device->device_id <= 0x4399) {
                return create_ac97_driver(device);
            }
            break;
        case 0x1022: // AMD
            if (device->device_id >= 0x7440 && device->device_id <= 0x7469) {
                return create_ac97_driver(device);
            }
            break;
    }
    
    // Generic AC'97/HDA controller'ları için
    if (device->class_code == 0x04 && device->subclass == 0x03) {
        // Audio controller
        if (device->prog_if == 0x00) {
            return create_ac97_driver(device);
        } else if (device->prog_if == 0x01) {
            return create_hda_driver(device);
        }
    }
    
    printf("Desteklenmeyen audio aygıtı: %04X:%04X\n", device->vendor_id, device->device_id);
    return NULL;
}

driver_t* create_usb_driver(pci_device_t* device) {
    // USB sürücüsü oluştur
    printf("USB sürücüsü oluşturuluyor...\n");
    
    // USB HID aygıtlarını kontrol et
    if (device->class_code == 0x03 && device->subclass == 0x01) {
        // USB HID Controller
        return create_usb_keyboard_driver(device);
    } else if (device->class_code == 0x03 && device->subclass == 0x02) {
        // USB Mouse
        return create_usb_mouse_driver(device);
    }
    
    return NULL;
}
