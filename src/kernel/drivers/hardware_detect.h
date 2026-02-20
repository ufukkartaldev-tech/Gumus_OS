#ifndef HARDWARE_DETECT_H
#define HARDWARE_DETECT_H

#include <stdint.h>
#include "driver.h"

// PCI Sabitleri
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// PCI Komutları
#define PCI_COMMAND_PORTIO       0x01
#define PCI_COMMAND_MMIO         0x02
#define PCI_COMMAND_BUS_MASTER   0x04
#define PCI_COMMAND_MEMORY_SPACE 0x02
#define PCI_COMMAND_INTERRUPT     0x400

// PCI Sınıf Kodları
#define PCI_CLASS_BRIDGE         0x06
#define PCI_CLASS_STORAGE        0x01
#define PCI_CLASS_NETWORK        0x02
#define PCI_CLASS_DISPLAY        0x03
#define PCI_CLASS_MULTIMEDIA      0x04
#define PCI_CLASS_SERIAL         0x0C

// PCI Alt Sınıf Kodları
#define PCI_SUBCLASS_BRIDGE_HOST 0x00
#define PCI_SUBCLASS_STORAGE_IDE 0x01
#define PCI_SUBCLASS_STORAGE_SATA 0x06
#define PCI_SUBCLASS_STORAGE_NVME 0x08
#define PCI_SUBCLASS_NETWORK_ETHERNET 0x00
#define PCI_SUBCLASS_DISPLAY_VGA 0x00
#define PCI_SUBCLASS_MULTIMEDIA_AUDIO 0x03

// PCI Aygıt Yapısı
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t  revision_id;
    uint8_t  prog_if;
    uint8_t  subclass;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
    uint32_t bar[6]; // Base Address Registers
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
    uint8_t  min_grant;
    uint8_t  max_latency;
} pci_device_t;

// Donanım Tespit Sonuçları
typedef struct {
    pci_device_t pci_devices[32];
    int pci_device_count;
    
    // Tespit edilen sürücüler
    driver_t* detected_drivers[16];
    int driver_count;
    
    // Donanım özeti
    int has_ide;
    int has_sata;
    int has_vga;
    int has_network;
    int has_audio;
    int has_usb;
} hardware_info_t;

// PCI Fonksiyonları
uint32_t pci_read_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_read_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t  pci_read_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_write_config_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
void pci_write_config_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
void pci_write_config_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

// Donanım Tespit Fonksiyonları
void hardware_detect_init();
void hardware_detect_scan_pci();
void hardware_detect_usb();
void hardware_detect_acpi();

// Sürücü Yükleme Fonksiyonları
int hardware_load_driver_for_device(pci_device_t* device);
driver_t* hardware_get_best_driver(uint16_t vendor_id, uint16_t device_id, uint8_t class_code, uint8_t subclass);

// Donanım Bilgileri
hardware_info_t* hardware_get_info();
void hardware_print_summary();

// Universal Sürücü Arayüzleri
driver_t* create_ide_driver(pci_device_t* device);
driver_t* create_sata_driver(pci_device_t* device);
driver_t* create_vga_driver(pci_device_t* device);
driver_t* create_network_driver(pci_device_t* device);
driver_t* create_audio_driver(pci_device_t* device);
driver_t* create_usb_driver(pci_device_t* device);

#endif
