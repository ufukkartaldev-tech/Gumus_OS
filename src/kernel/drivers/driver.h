#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

// Sürücü Tipleri
typedef enum {
    DRIVER_TYPE_CHAR,
    DRIVER_TYPE_BLOCK,
    DRIVER_TYPE_NET,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_INPUT
} driver_type_t;

// PCI Sınıf Kodları
typedef enum {
    PCI_CLASS_UNCLASSIFIED = 0x00,
    PCI_CLASS_STORAGE = 0x01,
    PCI_CLASS_NETWORK = 0x02,
    PCI_CLASS_DISPLAY = 0x03,
    PCI_CLASS_MULTIMEDIA = 0x04,
    PCI_CLASS_MEMORY = 0x05,
    PCI_CLASS_BRIDGE = 0x06,
    PCI_CLASS_COMMUNICATIONS = 0x07,
    PCI_CLASS_SYSTEM = 0x08,
    PCI_CLASS_INPUT = 0x09,
    PCI_CLASS_DOCKING = 0x0A,
    PCI_CLASS_PROCESSOR = 0x0B,
    PCI_CLASS_SERIAL = 0x0C,
    PCI_CLASS_WIRELESS = 0x0D,
    PCI_CLASS_INTELLIGENT = 0x0E,
    PCI_CLASS_SATELLITE = 0x0F,
    PCI_CLASS_ENCRYPTION = 0x10,
    PCI_CLASS_SIGNAL_PROCESSING = 0x11
} pci_class_t;

// PCI Alt Sınıf Kodları
typedef enum {
    PCI_SUBCLASS_STORAGE_IDE = 0x01,
    PCI_SUBCLASS_STORAGE_SATA = 0x06,
    PCI_SUBCLASS_DISPLAY_VGA = 0x00,
    PCI_SUBCLASS_NETWORK_ETHERNET = 0x00,
    PCI_SUBCLASS_MULTIMEDIA_AUDIO = 0x01,
    PCI_SUBCLASS_SERIAL_USB = 0x03
} pci_subclass_t;

// Sürücü Düğümü (Linked List için)
typedef struct driver_node {
    driver_t* driver;
    struct driver_node* next;
} driver_node_t;

// Sürücü Yöneticisi Yapısı
typedef struct {
    driver_node_t* driver_list;
    driver_node_t* active_list;
    int driver_count;
    int active_count;
    uint32_t next_unique_id;
} driver_manager_t;

// Sürücü Arayüzü (Interface)
typedef struct driver {
    char name[32];
    driver_type_t type;
    driver_class_t class;
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_device_id;
    uint32_t unique_id; // Her sürücü için unique ID
    
    // Fonksiyon İşaretçileri (Virtual Functions)
    int (*init)(void);
    int (*read)(void* buffer, uint32_t size, uint32_t offset);
    int (*write)(void* buffer, uint32_t size, uint32_t offset);
    int (*ioctl)(uint32_t command, void* arg);
    int (*shutdown)(void);
} driver_t;

// Sürücü Yöneticisi Fonksiyonları
void driver_manager_init();
void driver_manager_cleanup();
void driver_register(driver_t* driver);
driver_t* driver_get(const char* name);
driver_t* driver_find_by_id(uint16_t vendor_id, uint16_t device_id);
driver_t* driver_get_by_class(driver_class_t class);
driver_t* driver_get_by_type(driver_type_t type);
driver_t* driver_get_by_unique_id(uint32_t unique_id);
int driver_activate_by_id(uint32_t unique_id);
int driver_deactivate_by_id(uint32_t unique_id);
int driver_activate(const char* name);
int driver_deactivate(const char* name);
int driver_unregister(const char* name);
void driver_list_all();
void driver_list_active();
int driver_auto_load_all();

#endif
