#include "driver.h"

// Forward declaration for pci_device_t
typedef struct pci_device pci_device_t;

// Stub implementations for missing USB drivers
driver_t* create_ohci_driver(pci_device_t* device) {
    (void)device;
    return NULL;
}

driver_t* create_ehci_driver(pci_device_t* device) {
    (void)device;
    return NULL;
}

driver_t* create_xhci_driver(pci_device_t* device) {
    (void)device;
    return NULL;
}
