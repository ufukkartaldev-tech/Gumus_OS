#include "ohci.h"
#include "../core/memory.h"
#include "../core/pci.h"
#include "../core/interrupts.h"
#include "../core/io.h"
#include "usb_host.h"
#include "string.h"
#include "stdio.h"

// Global OHCI controller list
static ohci_controller_t* ohci_controllers = NULL;

// Memory alignment helpers
static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// OHCI register access functions
static inline uint32_t ohci_read_reg(ohci_controller_t* controller, uint32_t offset) {
    return controller->registers[offset / 4];
}

static inline void ohci_write_reg(ohci_controller_t* controller, uint32_t offset, uint32_t value) {
    controller->registers[offset / 4] = value;
}

// HCCA allocation
static int ohci_allocate_hcca(ohci_controller_t* controller) {
    // HCCA must be 256-byte aligned
    controller->hcca = (ohci_hcca_t*)malloc(sizeof(ohci_hcca_t) + 255);
    if (!controller->hcca) {
        printf("OHCI: HCCA bellek tahsisatı başarısız\n");
        return -1;
    }
    
    // Align to 256-byte boundary
    uint32_t addr = (uint32_t)controller->hcca;
    controller->hcca = (ohci_hcca_t*)align_up(addr, 256);
    
    // Initialize HCCA
    memset(controller->hcca, 0, sizeof(ohci_hcca_t));
    
    // Set HCCA address in controller
    ohci_write_reg(controller, OHCI_HCCA, (uint32_t)controller->hcca);
    
    return 0;
}

// Endpoint and Transfer Descriptor management
static ohci_ed_t* ohci_allocate_ed(ohci_controller_t* controller) {
    if (controller->pool_index >= 1000) { // Arbitrary limit
        return NULL;
    }
    
    ohci_ed_t* ed = (ohci_ed_t*)(controller->ed_pool + controller->pool_index * sizeof(ohci_ed_t));
    memset(ed, 0, sizeof(ohci_ed_t));
    controller->pool_index++;
    
    return ed;
}

static ohci_td_t* ohci_allocate_td(ohci_controller_t* controller) {
    if (controller->pool_index >= 1000) {
        return NULL;
    }
    
    ohci_td_t* td = (ohci_td_t*)(controller->td_pool + controller->pool_index * sizeof(ohci_td_t));
    memset(td, 0, sizeof(ohci_td_t));
    controller->pool_index++;
    
    return td;
}

// OHCI controller reset
static int ohci_controller_reset(ohci_controller_t* controller) {
    printf("OHCI: Controller reset başlatılıyor\n");
    
    // Wait for controller to be ready
    uint32_t timeout = 10000;
    while (timeout-- && (ohci_read_reg(controller, OHCI_CONTROL) & OHCI_CTRL_HCFS) != OHCI_STATE_SUSPEND) {
        // Wait
    }
    
    if (timeout == 0) {
        printf("OHCI: Controller hazır değil\n");
        return -1;
    }
    
    // Reset the controller
    ohci_write_reg(controller, OHCI_CMD_STATUS, OHCI_CMD_HCR);
    
    // Wait for reset to complete
    timeout = 10000;
    while (timeout-- && (ohci_read_reg(controller, OHCI_CMD_STATUS) & OHCI_CMD_HCR)) {
        // Wait
    }
    
    if (timeout == 0) {
        printf("OHCI: Reset tamamlanamadı\n");
        return -1;
    }
    
    printf("OHCI: Controller reset tamamlandı\n");
    return 0;
}

// OHCI controller initialization
static int ohci_controller_init(ohci_controller_t* controller) {
    printf("OHCI: Controller başlatılıyor\n");
    
    // Allocate memory pools
    controller->ed_pool = (uint32_t)malloc(1000 * sizeof(ohci_ed_t) + 255);
    controller->td_pool = (uint32_t)malloc(1000 * sizeof(ohci_td_t) + 255);
    
    if (!controller->ed_pool || !controller->td_pool) {
        printf("OHCI: Bellek havuzları oluşturulamadı\n");
        return -1;
    }
    
    // Align memory pools
    controller->ed_pool = align_up(controller->ed_pool, 256);
    controller->td_pool = align_up(controller->td_pool, 256);
    
    controller->pool_index = 0;
    
    // Allocate HCCA
    if (ohci_allocate_hcca(controller) != 0) {
        return -1;
    }
    
    // Reset controller
    if (ohci_controller_reset(controller) != 0) {
        return -1;
    }
    
    // Set frame interval
    uint32_t fm_interval = ohci_read_reg(controller, OHCI_FM_INTERVAL);
    fm_interval &= ~0x3FFF;
    fm_interval |= 0x2EDF; // Default value
    ohci_write_reg(controller, OHCI_FM_INTERVAL, fm_interval);
    
    // Enable interrupts
    uint32_t interrupt_enable = OHCI_INTR_MIE | OHCI_INTR_RHSC | OHCI_INTR_WDH;
    ohci_write_reg(controller, OHCI_INTERRUPT_ENABLE, interrupt_enable);
    
    // Start controller
    uint32_t control = ohci_read_reg(controller, OHCI_CONTROL);
    control &= ~OHCI_CTRL_HCFS;
    control |= OHCI_STATE_OPERATIONAL;
    control |= OHCI_CTRL_CLE | OHCI_CTRL_BLE | OHCI_CTRL_PLE; // Enable lists
    ohci_write_reg(controller, OHCI_CONTROL, control);
    
    // Get number of ports
    uint32_t rh_desc_a = ohci_read_reg(controller, OHCI_RH_DESCRIPTOR_A);
    controller->num_ports = (rh_desc_a & 0xFF);
    
    printf("OHCI: Controller başlatıldı, %d port\n", controller->num_ports);
    return 0;
}

// OHCI device enumeration
static int ohci_enumerate_device_impl(usb_host_controller_t* base_controller, uint8_t port) {
    ohci_controller_t* controller = (ohci_controller_t*)base_controller;
    
    printf("OHCI: Aygıt enumeration başlatılıyor (port %d)\n", port);
    
    // Check if port is valid
    if (port >= controller->num_ports) {
        printf("OHCI: Geçersiz port: %d\n", port);
        return -1;
    }
    
    // Check port status
    uint32_t port_status = ohci_read_reg(controller, OHCI_RH_PORT_STATUS + (port * 4));
    
    if (!(port_status & OHCI_PORT_CCS)) {
        printf("OHCI: Port %d'de aygıt bağlı değil\n", port);
        return -1;
    }
    
    // Reset port
    printf("OHCI: Port %d resetleniyor\n", port);
    ohci_write_reg(controller, OHCI_RH_PORT_STATUS + (port * 4), OHCI_PORT_PRS);
    
    // Wait for reset to complete
    uint32_t timeout = 10000;
    while (timeout-- && (ohci_read_reg(controller, OHCI_RH_PORT_STATUS + (port * 4)) & OHCI_PORT_PRS)) {
        // Wait
    }
    
    if (timeout == 0) {
        printf("OHCI: Port reset tamamlanamadı\n");
        return -1;
    }
    
    // Clear reset status change
    ohci_write_reg(controller, OHCI_RH_PORT_STATUS + (port * 4), OHCI_PORT_PRSC);
    
    // Check if device is enabled
    port_status = ohci_read_reg(controller, OHCI_RH_PORT_STATUS + (port * 4));
    if (!(port_status & OHCI_PORT_PES)) {
        printf("OHCI: Aygıt enable edilemedi\n");
        return -1;
    }
    
    // Determine device speed
    usb_speed_t speed = (port_status & OHCI_PORT_LSDA) ? USB_SPEED_LOW : USB_SPEED_FULL;
    printf("OHCI: Aygıt hız: %s\n", speed == USB_SPEED_LOW ? "Low" : "Full");
    
    // TODO: Complete device enumeration
    // - Set device address
    // - Get device descriptor
    // - Get configuration descriptor
    // - Set configuration
    
    printf("OHCI: Aygıt enumeration tamamlandı\n");
    return 0;
}

// OHCI control transfer
static int ohci_control_transfer_impl(usb_host_controller_t* base_controller, 
                                     uint8_t device_addr, uint8_t endpoint,
                                     uint8_t* setup_packet, uint8_t* data, uint32_t length) {
    ohci_controller_t* controller = (ohci_controller_t*)base_controller;
    
    // Allocate ED and TD
    ohci_ed_t* ed = ohci_allocate_ed(controller);
    ohci_td_t* td_setup = ohci_allocate_td(controller);
    ohci_td_t* td_data = NULL;
    ohci_td_t* td_status = ohci_allocate_td(controller);
    
    if (!ed || !td_setup || !td_status) {
        printf("OHCI: Descriptor tahsis hatası\n");
        return -1;
    }
    
    // Setup ED
    ed->control = (device_addr & OHCI_ED_FA) | 
                  ((endpoint & 0xF) << 7) |
                  OHCI_ED_DIR_OUT |
                  (64 << 16); // Max packet size
    
    // Setup TD for setup packet
    td_setup->control = OHCI_TD_T0 | (3 << 21); // 3 errors allowed
    td_setup->cbp = (uint32_t)setup_packet;
    td_setup->be = (uint32_t)setup_packet + 7;
    td_setup->next_td = (uint32_t)td_status;
    
    // Data TD if needed
    if (data && length > 0) {
        td_data = ohci_allocate_td(controller);
        if (!td_data) {
            printf("OHCI: Data TD tahsis hatası\n");
            return -1;
        }
        
        td_setup->next_td = (uint32_t)td_data;
        td_data->control = OHCI_TD_T1 | (3 << 21);
        td_data->cbp = (uint32_t)data;
        td_data->be = (uint32_t)data + length - 1;
        td_data->next_td = (uint32_t)td_status;
    }
    
    // Status TD
    td_status->control = OHCI_TD_T1 | (3 << 21);
    td_status->cbp = 0;
    td_status->be = 0;
    td_status->next_td = 0;
    
    // Link ED to control list
    ed->head_td = (uint32_t)td_setup;
    ed->tail_td = 0;
    
    // Add to control list
    ohci_write_reg(controller, OHCI_CONTROL_HEAD_ED, (uint32_t)ed);
    ohci_write_reg(controller, OHCI_CMD_STATUS, OHCI_CMD_CLF);
    
    // Wait for completion
    uint32_t timeout = 10000;
    while (timeout-- && (td_status->control & OHCI_TD_CC) == OHCI_TD_NOTACCESSED) {
        // Wait
    }
    
    // Check result
    uint32_t condition_code = td_status->control & OHCI_TD_CC;
    if (condition_code != OHCI_TD_NOERROR) {
        printf("OHCI: Control transfer başarısız, kod: %d\n", condition_code);
        return -1;
    }
    
    return 0;
}

// OHCI bulk transfer
static int ohci_bulk_transfer_impl(usb_host_controller_t* base_controller,
                                  uint8_t device_addr, uint8_t endpoint,
                                  uint8_t* data, uint32_t length, uint8_t direction) {
    ohci_controller_t* controller = (ohci_controller_t*)base_controller;
    
    // Allocate ED and TD
    ohci_ed_t* ed = ohci_allocate_ed(controller);
    ohci_td_t* td = ohci_allocate_td(controller);
    
    if (!ed || !td) {
        printf("OHCI: Descriptor tahsis hatası\n");
        return -1;
    }
    
    // Setup ED
    ed->control = (device_addr & OHCI_ED_FA) | 
                  ((endpoint & 0xF) << 7) |
                  (direction ? OHCI_ED_DIR_IN : OHCI_ED_DIR_OUT) |
                  (64 << 16); // Max packet size
    
    // Setup TD
    td->control = OHCI_TD_T1 | (3 << 21);
    td->cbp = (uint32_t)data;
    td->be = (uint32_t)data + length - 1;
    td->next_td = 0;
    
    // Link ED to bulk list
    ed->head_td = (uint32_t)td;
    ed->tail_td = 0;
    
    // Add to bulk list
    ohci_write_reg(controller, OHCI_BULK_HEAD_ED, (uint32_t)ed);
    ohci_write_reg(controller, OHCI_CMD_STATUS, OHCI_CMD_BLF);
    
    // Wait for completion
    uint32_t timeout = 10000;
    while (timeout-- && (td->control & OHCI_TD_CC) == OHCI_TD_NOTACCESSED) {
        // Wait
    }
    
    // Check result
    uint32_t condition_code = td->control & OHCI_TD_CC;
    if (condition_code != OHCI_TD_NOERROR) {
        printf("OHCI: Bulk transfer başarısız, kod: %d\n", condition_code);
        return -1;
    }
    
    return 0;
}

// OHCI interrupt transfer
static int ohci_interrupt_transfer_impl(usb_host_controller_t* base_controller,
                                       uint8_t device_addr, uint8_t endpoint,
                                       uint8_t* data, uint32_t length) {
    ohci_controller_t* controller = (ohci_controller_t*)base_controller;
    
    // For now, use bulk transfer as a fallback
    return ohci_bulk_transfer_impl(base_controller, device_addr, endpoint, data, length, USB_DIRECTION_IN);
}

// OHCI interrupt handler
void ohci_irq_handler() {
    printf("OHCI: Interrupt alındı\n");
    
    ohci_controller_t* current = ohci_controllers;
    while (current) {
        uint32_t interrupt_status = ohci_read_reg(current, OHCI_INTERRUPT_STATUS);
        
        if (interrupt_status) {
            // Clear interrupts
            ohci_write_reg(current, OHCI_INTERRUPT_STATUS, interrupt_status);
            
            // Handle root hub status change
            if (interrupt_status & OHCI_INTR_RHSC) {
                printf("OHCI: Root hub status change\n");
                // TODO: Handle port connect/disconnect
            }
            
            // Handle done head
            if (interrupt_status & OHCI_INTR_WDH) {
                printf("OHCI: Transfer tamamlandı\n");
                // TODO: Process completed transfers
            }
        }
        
        current = current->base.next;
    }
}

// OHCI driver initialization function
int ohci_init(usb_host_controller_t* controller) {
    ohci_controller_t* ohci_ctrl = (ohci_controller_t*)controller;
    
    printf("OHCI: Sürücü başlatılıyor\n");
    
    // Map MMIO registers
    pci_device_t* pci_dev = (pci_device_t*)controller->mmio_base;
    ohci_ctrl->registers = (volatile uint32_t*)pci_dev->bar[0] & ~0xF;
    
    if (!ohci_ctrl->registers) {
        printf("OHCI: Register haritalaması başarısız\n");
        return -1;
    }
    
    // Initialize controller
    return ohci_controller_init(ohci_ctrl);
}

// OHCI reset function
int ohci_reset(usb_host_controller_t* controller) {
    ohci_controller_t* ohci_ctrl = (ohci_controller_t*)controller;
    return ohci_controller_reset(ohci_ctrl);
}

// Create OHCI driver
driver_t* create_ohci_driver(pci_device_t* device) {
    ohci_controller_t* controller = malloc(sizeof(ohci_controller_t));
    if (!controller) {
        return NULL;
    }
    
    memset(controller, 0, sizeof(ohci_controller_t));
    
    // Setup base driver
    controller->base.type = USB_HC_OHCI;
    controller->base.init = (int (*)(void*))ohci_init;
    controller->base.reset = (int (*)(void*))ohci_reset;
    controller->base.enumerate_device = ohci_enumerate_device_impl;
    controller->base.control_transfer = ohci_control_transfer_impl;
    controller->base.bulk_transfer = ohci_bulk_transfer_impl;
    controller->base.interrupt_transfer = ohci_interrupt_transfer_impl;
    
    controller->base.mmio_base = device;
    controller->base.irq_line = device->irq;
    
    // Copy PCI info
    controller->base.vendor_id = device->vendor_id;
    controller->base.device_id = device->device_id;
    
    strcpy(controller->base.name, "OHCI USB Host Controller");
    controller->base.type = DRIVER_TYPE_INPUT;
    controller->base.class = DRIVER_CLASS_SERIAL;
    
    // Add to global list
    controller->base.next = (struct usb_host_controller*)ohci_controllers;
    ohci_controllers = controller;
    
    return (driver_t*)controller;
}
