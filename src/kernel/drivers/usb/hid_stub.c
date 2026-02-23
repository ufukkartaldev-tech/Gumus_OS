#include "driver.h"

// Forward declaration for pci_device_t
typedef struct pci_device pci_device_t;

// USB HID driver stub
driver_t* create_usb_hid_driver(pci_device_t* device) {
    (void)device;
    return NULL;
}
