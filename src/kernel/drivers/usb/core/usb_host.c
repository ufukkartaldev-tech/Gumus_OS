#include "usb_host.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "printf.h"
#include "stdio.h"

// External stub driver functions
extern driver_t* create_ohci_driver(pci_device_t* device);
extern driver_t* create_ehci_driver(pci_device_t* device);
extern driver_t* create_xhci_driver(pci_device_t* device);

// Global USB Host Manager
static usb_host_controller_t* usb_controllers = NULL;
static usb_device_t* usb_devices = NULL;
static uint32_t next_device_id = 1;
static uint32_t next_controller_id = 1;

// USB Host Controller fonksiyonlarÄ±
void usb_host_init() {
    printf("USB Host Controller sistemi baÅŸlatÄ±lÄ±yor...\n");
    
    // PCI Ã¼zerinden USB Host Controller'larÄ± tara
    hardware_info_t* hw_info = hardware_get_info();
    
    for (int i = 0; i < hw_info->pci_device_count; i++) {
        pci_device_t* device = &hw_info->pci_devices[i];
        
        // USB Host Controller sÄ±nÄ±f kodlarÄ±nÄ± kontrol et
        if (device->class_code == 0x0C && device->subclass == 0x03) {
            driver_t* driver = NULL;
            
            switch (device->prog_if) {
                case 0x00: // UHCI
                    printf("UHCI Controller bulundu: %04X:%04X\n", 
                           device->vendor_id, device->device_id);
                    break;
                    
                case 0x10: // OHCI
                    printf("OHCI Controller bulundu: %04X:%04X\n", 
                           device->vendor_id, device->device_id);
                    driver = create_ohci_driver(device);
                    break;
                    
                case 0x20: // EHCI
                    printf("EHCI Controller bulundu: %04X:%04X\n", 
                           device->vendor_id, device->device_id);
                    driver = create_ehci_driver(device);
                    break;
                    
                case 0x30: // XHCI
                    printf("XHCI Controller bulundu: %04X:%04X\n", 
                           device->vendor_id, device->device_id);
                    driver = create_xhci_driver(device);
                    break;
                    
                default:
                    printf("Bilinmeyen USB Controller: %04X:%04X (IF: %02X)\n", 
                           device->vendor_id, device->device_id, device->prog_if);
                    break;
            }
            
            if (driver) {
                driver_register(driver);
                driver_activate(driver->name);
            }
        }
    }
    
    printf("USB Host Controller sistemi baÅŸlatÄ±ldÄ±\n");
}

int usb_host_register_controller(usb_host_controller_t* controller) {
    if (!controller) {
        printf("usb_host_register_controller: BoÅŸ controller\n");
        return -1;
    }
    
    // Controller listesine ekle
    controller->next = usb_controllers;
    usb_controllers = controller;
    
    // Controller'Ä± baÅŸlat
    if (controller->init && controller->init(controller) == 0) {
        printf("USB Host Controller kaydedildi: %s (ID: %d)\n", 
               controller->base.name, controller->base.unique_id);
        
        // PortlarÄ± tara ve aygÄ±tlarÄ± enumerate et
        for (uint32_t port = 0; port < controller->num_ports; port++) {
            usb_host_enumerate_device(controller, port);
        }
        
        return 0;
    }
    
    printf("USB Host Controller baÅŸlatÄ±lamadÄ±: %s\n", controller->base.name);
    return -1;
}

int usb_host_unregister_controller(usb_host_controller_t* controller) {
    if (!controller) {
        return -1;
    }
    
    // Controller'Ä± kapat
    if (controller->base.shutdown) {
        controller->base.shutdown();
    }
    
    // Listeden Ã§Ä±kar
    usb_host_controller_t** current = &usb_controllers;
    while (*current) {
        if (*current == controller) {
            *current = controller->next;
            printf("USB Host Controller kaldÄ±rÄ±ldÄ±: %s\n", controller->base.name);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;
}

int usb_host_enumerate_device(usb_host_controller_t* controller, uint8_t port) {
    if (!controller) {
        return -1;
    }
    
    // Controller'Ä±n enumerate fonksiyonunu Ã§aÄŸÄ±r
    if (controller->enumerate_device) {
        int result = controller->enumerate_device(controller, port);
        if (result == 0) {
            printf("USB aygÄ±tÄ± enumerate edildi: Controller %s, Port %d\n", 
                   controller->base.name, port);
        }
        return result;
    }
    
    return -1;
}

int usb_host_control_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                           uint8_t endpoint, usb_setup_packet_t* setup,
                           uint8_t* data, uint32_t length) {
    if (!controller || !setup) {
        return -1;
    }
    
    if (controller->control_transfer) {
        return controller->control_transfer(controller, device_addr, endpoint,
                                       (uint8_t*)setup, data, length);
    }
    
    return -1;
}

int usb_host_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                        uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction) {
    if (!controller || !data) {
        return -1;
    }
    
    if (controller->bulk_transfer) {
        return controller->bulk_transfer(controller, device_addr, endpoint,
                                    data, length, direction);
    }
    
    return -1;
}

int usb_host_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                            uint8_t endpoint, uint8_t* data, uint32_t length) {
    if (!controller || !data) {
        return -1;
    }
    
    if (controller->interrupt_transfer) {
        return controller->interrupt_transfer(controller, device_addr, endpoint,
                                        data, length);
    }
    
    return -1;
}

void usb_host_list_devices() {
    printf("\n=== USB AygÄ±tlarÄ± ===\n");
    usb_device_t* current = usb_devices;
    int count = 1;
    
    while (current) {
        printf("%d. AygÄ±t ID: %d, Adres: %d, Port: %d, Speed: %d\n", 
               count++, current->id, current->address, current->port, current->speed);
        printf("   Vendor: %04X, Product: %04X, Class: %02X\n",
               current->device_desc.vendor_id, current->device_desc.product_id,
               current->device_desc.class_code);
        current = current->next;
    }
    printf("====================\n");
}

void usb_host_list_controllers() {
    printf("\n=== USB Host Controller'larÄ± ===\n");
    usb_host_controller_t* current = usb_controllers;
    int count = 1;
    
    while (current) {
        printf("%d. %s (Type: %d, Ports: %d, IRQ: %d)\n", 
               count++, current->base.name, current->type, 
               current->num_ports, current->irq_line);
        current = current->next;
    }
    printf("=============================\n");
}

// USB aygÄ±t ekleme fonksiyonu
int usb_host_add_device(usb_device_t* device) {
    if (!device) {
        return -1;
    }
    
    // ID ata
    device->id = next_device_id++;
    
    // Listeye ekle
    device->next = usb_devices;
    usb_devices = device;
    
    printf("USB aygÄ±tÄ± eklendi: ID %d, Address %d\n", 
           device->id, device->address);
    
    return 0;
}

// USB aygÄ±t kaldÄ±rma fonksiyonu
int usb_host_remove_device(uint32_t device_id) {
    usb_device_t** current = &usb_devices;
    
    while (*current) {
        if ((*current)->id == device_id) {
            usb_device_t* to_remove = *current;
            *current = (*current)->next;
            printf("USB aygÄ±tÄ± kaldÄ±rÄ±ldÄ±: ID %d\n", device_id);
            free(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;
}

// USB descriptor okuma fonksiyonu
int usb_host_get_descriptor(usb_host_controller_t* controller, uint8_t device_addr,
                         uint8_t type, uint8_t index, uint8_t* buffer, uint16_t length) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x80; // Device to host
    setup.request = USB_REQ_GET_DESCRIPTOR;
    setup.value = (type << 8) | index;
    setup.index = 0;
    setup.length = length;
    
    return usb_host_control_transfer(controller, device_addr, 0, &setup, buffer, length);
}
