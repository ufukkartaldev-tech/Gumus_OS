#include "ohci.h"
#include "../core/memory.h"
#include "../core/io.h"
#include "../core/string.h"
#include "../core/printf.h"
#include "../core/interrupt.h"

// OHCI Controller implementasyonu
static ohci_controller_t* ohci_controllers = NULL;

// OHCI register okuma/yazma
static inline uint32_t ohci_read_reg(ohci_controller_t* ohci, uint32_t offset) {
    return ohci->registers[offset / 4];
}

static inline void ohci_write_reg(ohci_controller_t* ohci, uint32_t offset, uint32_t value) {
    ohci->registers[offset / 4] = value;
}

// OHCI başlatma
int ohci_init(usb_host_controller_t* controller) {
    ohci_controller_t* ohci = (ohci_controller_t*)controller;
    
    printf("OHCI Controller başlatılıyor: %s\n", controller->name);
    
    // Controller'ı reset state'ine getir
    uint32_t control = ohci_read_reg(ohci, OHCI_CONTROL);
    ohci_write_reg(ohci, OHCI_CONTROL, (control & ~OHCI_CTRL_HCFS) | OHCI_STATE_RESET);
    
    // Operational state'ine geç
    ohci_write_reg(ohci, OHCI_CONTROL, (control & ~OHCI_CTRL_HCFS) | OHCI_STATE_OPERATIONAL);
    
    // HCCA ayarla
    ohci->hcca = (ohci_hcca_t*)malloc(sizeof(ohci_hcca_t));
    if (!ohci->hcca) return -1;
    memset(ohci->hcca, 0, sizeof(ohci_hcca_t));
    ohci_write_reg(ohci, OHCI_HCCA, (uint32_t)ohci->hcca);
    
    // Memory pool oluştur
    ohci->ed_pool = (uint32_t)malloc(1024 * sizeof(ohci_ed_t));
    ohci->td_pool = (uint32_t)malloc(1024 * sizeof(ohci_td_t));
    ohci->pool_index = 0;
    
    if (!ohci->ed_pool || !ohci->td_pool) return -1;
    
    // Interrupt'ları etkinleştir
    ohci_write_reg(ohci, OHCI_INTERRUPT_ENABLE, 
                   OHCI_INTR_MIE | OHCI_INTR_WDH | OHCI_INTR_RHSC);
    
    // Port sayısını al
    uint32_t rh_desc_a = ohci_read_reg(ohci, OHCI_RH_DESCRIPTOR_A);
    ohci->num_ports = (rh_desc_a & 0xFF);
    ohci->revision = ohci_read_reg(ohci, OHCI_REVISION) & 0xFF;
    
    printf("OHCI Controller başlatıldı: Rev %d.%d, %d port\n",
           (ohci->revision >> 4) & 0xF, ohci->revision & 0xF, ohci->num_ports);
    
    return 0;
}

// OHCI sürücü oluşturma
driver_t* create_ohci_driver(pci_device_t* device) {
    if (!device) return NULL;
    
    ohci_controller_t* ohci = malloc(sizeof(ohci_controller_t));
    if (!ohci) return NULL;
    
    memset(ohci, 0, sizeof(ohci_controller_t));
    
    // Sürücü yapısını ayarla
    strcpy(ohci->base.base.name, "OHCI USB Host Controller");
    ohci->base.base.type = DRIVER_TYPE_CHAR;
    ohci->base.base.class = DRIVER_CLASS_SERIAL;
    ohci->base.base.vendor_id = device->vendor_id;
    ohci->base.base.device_id = device->device_id;
    ohci->base.type = USB_HC_OHCI;
    
    // Fonksiyonları ayarla
    ohci->base.init = (int(*)(void*))ohci_init;
    ohci->base.reset = (int(*)(void*))ohci_reset;
    ohci->base.enumerate_device = ohci_enumerate_device;
    ohci->base.control_transfer = ohci_control_transfer;
    ohci->base.bulk_transfer = ohci_bulk_transfer;
    ohci->base.interrupt_transfer = ohci_interrupt_transfer;
    
    // MMIO base adresini ayarla
    uint32_t bar = device->bar[0];
    if (!(bar & 0x01)) {
        ohci->registers = (volatile uint32_t*)bar;
        ohci->base.mmio_base = bar;
    } else {
        printf("OHCI: I/O mapped desteklenmiyor\n");
        free(ohci);
        return NULL;
    }
    
    printf("OHCI sürücüsü oluşturuldu: %04X:%04X\n",
           device->vendor_id, device->device_id);
    
    return &ohci->base.base;
}
