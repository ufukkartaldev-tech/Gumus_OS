#include "usb_host.h"
#include "../core/memory.h"
#include "../core/string.h"
#include "../core/printf.h"

// EHCI Sürücüsü (Placeholder)
driver_t* create_ehci_driver(pci_device_t* device) {
    if (!device) return NULL;
    
    driver_t* driver = malloc(sizeof(driver_t));
    if (!driver) return NULL;
    
    memset(driver, 0, sizeof(driver_t));
    
    strcpy(driver->name, "EHCI USB Host Controller");
    driver->type = DRIVER_TYPE_CHAR;
    driver->class = DRIVER_CLASS_SERIAL;
    driver->vendor_id = device->vendor_id;
    driver->device_id = device->device_id;
    driver->unique_id = 0; // driver_register'da atanacak
    
    printf("EHCI sürücüsü oluşturuldu: %04X:%04X\n",
           device->vendor_id, device->device_id);
    
    return driver;
}

// XHCI Sürücüsü (Placeholder)
driver_t* create_xhci_driver(pci_device_t* device) {
    if (!device) return NULL;
    
    driver_t* driver = malloc(sizeof(driver_t));
    if (!driver) return NULL;
    
    memset(driver, 0, sizeof(driver_t));
    
    strcpy(driver->name, "XHCI USB Host Controller");
    driver->type = DRIVER_TYPE_CHAR;
    driver->class = DRIVER_CLASS_SERIAL;
    driver->vendor_id = device->vendor_id;
    driver->device_id = device->device_id;
    driver->unique_id = 0; // driver_register'da atanacak
    
    printf("XHCI sürücüsü oluşturuldu: %04X:%04X\n",
           device->vendor_id, device->device_id);
    
    return driver;
}
