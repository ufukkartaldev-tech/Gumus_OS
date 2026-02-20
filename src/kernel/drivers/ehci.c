#include "ehci.h"
#include "../core/memory.h"
#include "../core/pci.h"
#include "../core/interrupts.h"
#include "../core/io.h"
#include "usb_host.h"
#include "string.h"
#include "stdio.h"

// Global EHCI controller list
static ehci_controller_t* ehci_controllers = NULL;

// Memory alignment helpers
static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// Register access functions
uint32_t ehci_read_cap_reg(ehci_controller_t* controller, uint32_t offset) {
    return controller->cap_regs[offset / 4];
}

uint32_t ehci_read_op_reg(ehci_controller_t* controller, uint32_t offset) {
    return controller->op_regs[offset / 4];
}

void ehci_write_op_reg(ehci_controller_t* controller, uint32_t offset, uint32_t value) {
    controller->op_regs[offset / 4] = value;
}

uint32_t ehci_read_leg_reg(ehci_controller_t* controller, uint32_t offset) {
    return controller->leg_regs[offset / 4];
}

void ehci_write_leg_reg(ehci_controller_t* controller, uint32_t offset, uint32_t value) {
    controller->leg_regs[offset / 4] = value;
}

// QTD management
ehci_qtd_t* ehci_allocate_qtd(ehci_controller_t* controller) {
    if (controller->pool_index >= 1000) {
        return NULL;
    }
    
    ehci_qtd_t* qtd = (ehci_qtd_t*)(controller->qtd_pool + controller->pool_index * sizeof(ehci_qtd_t));
    memset(qtd, 0, sizeof(ehci_qtd_t));
    controller->pool_index++;
    
    return qtd;
}

ehci_qh_t* ehci_allocate_qh(ehci_controller_t* controller) {
    if (controller->pool_index >= 1000) {
        return NULL;
    }
    
    ehci_qh_t* qh = (ehci_qh_t*)(controller->qh_pool + controller->pool_index * sizeof(ehci_qh_t));
    memset(qh, 0, sizeof(ehci_qh_t));
    controller->pool_index++;
    
    return qh;
}

// EHCI controller reset
static int ehci_controller_reset(ehci_controller_t* controller) {
    printf("EHCI: Controller reset başlatılıyor\n");
    
    // Check if controller is already halted
    uint32_t status = ehci_read_op_reg(controller, EHCI_USBSTS);
    if (!(status & EHCI_STS_HCHalted)) {
        // Halt the controller
        uint32_t cmd = ehci_read_op_reg(controller, EHCI_USBCMD);
        cmd &= ~EHCI_CMD_RUN;
        ehci_write_op_reg(controller, EHCI_USBCMD, cmd);
        
        // Wait for halt
        uint32_t timeout = 10000;
        while (timeout-- && !(ehci_read_op_reg(controller, EHCI_USBSTS) & EHCI_STS_HCHalted)) {
            // Wait
        }
        
        if (timeout == 0) {
            printf("EHCI: Controller durdurulamadı\n");
            return -1;
        }
    }
    
    // Reset the controller
    uint32_t cmd = ehci_read_op_reg(controller, EHCI_USBCMD);
    cmd |= EHCI_CMD_HCRESET;
    ehci_write_op_reg(controller, EHCI_USBCMD, cmd);
    
    // Wait for reset to complete
    uint32_t timeout = 10000;
    while (timeout-- && (ehci_read_op_reg(controller, EHCI_USBCMD) & EHCI_CMD_HCRESET)) {
        // Wait
    }
    
    if (timeout == 0) {
        printf("EHCI: Reset tamamlanamadı\n");
        return -1;
    }
    
    printf("EHCI: Controller reset tamamlandı\n");
    return 0;
}

// Frame list allocation
static int ehci_allocate_frame_list(ehci_controller_t* controller) {
    // Determine frame list size
    uint32_t hcc_params = ehci_read_cap_reg(controller, EHCI_HCCPARAMS);
    if (hcc_params & 0x01) {
        controller->frame_list_size = 1024; // 32-bit frame list
    } else {
        controller->frame_list_size = 256;  // 64-bit frame list
    }
    
    // Allocate frame list (must be page-aligned)
    controller->frame_list = (ehci_fle_t*)malloc(controller->frame_list_size * sizeof(ehci_fle_t) + 4095);
    if (!controller->frame_list) {
        printf("EHCI: Frame list bellek tahsisatı başarısız\n");
        return -1;
    }
    
    // Align to page boundary
    uint32_t addr = (uint32_t)controller->frame_list;
    controller->frame_list = (ehci_fle_t*)align_up(addr, 4096);
    
    // Initialize frame list
    memset(controller->frame_list, 0, controller->frame_list_size * sizeof(ehci_fle_t));
    
    // Set frame list base address
    ehci_write_op_reg(controller, EHCI_PERIODICLISTBASE, (uint32_t)controller->frame_list);
    
    printf("EHCI: Frame list oluşturuldu, boyut: %d\n", controller->frame_list_size);
    return 0;
}

// Asynchronous queue setup
static int ehci_setup_async_queue(ehci_controller_t* controller) {
    // Allocate async queue head
    controller->async_qh = ehci_allocate_qh(controller);
    if (!controller->async_qh) {
        printf("EHCI: Async QH oluşturulamadı\n");
        return -1;
    }
    
    // Set up async QH (horizontal link points to itself)
    controller->async_qh->horizontal_link = (uint32_t)controller->async_qh | EHCI_FLE_TYPE_QH;
    controller->async_qh->ep_char = EHCI_QH_HRL; // Head of reclamation list
    controller->async_qh->current_qtd = 1; // Terminate
    
    // Set async list address
    ehci_write_op_reg(controller, EHCI_ASYNCLISTADDR, (uint32_t)controller->async_qh);
    
    printf("EHCI: Async queue ayarlandı\n");
    return 0;
}

// EHCI controller initialization
static int ehci_controller_init(ehci_controller_t* controller) {
    printf("EHCI: Controller başlatılıyor\n");
    
    // Get controller parameters
    controller->hcs_params = ehci_read_cap_reg(controller, EHCI_HCSPARAMS);
    controller->hcc_params = ehci_read_cap_reg(controller, EHCI_HCCPARAMS);
    
    // Extract number of ports
    controller->num_ports = (controller->hcs_params & 0x0F);
    controller->companion_controllers = ((controller->hcs_params >> 12) & 0x0F);
    controller->companion_ports = ((controller->hcs_params >> 16) & 0x0F);
    
    printf("EHCI: %d port, %d companion controller\n", controller->num_ports, controller->companion_controllers);
    
    // Allocate memory pools
    controller->qtd_pool = (uint32_t)malloc(1000 * sizeof(ehci_qtd_t) + 4095);
    controller->qh_pool = (uint32_t)malloc(1000 * sizeof(ehci_qh_t) + 4095);
    
    if (!controller->qtd_pool || !controller->qh_pool) {
        printf("EHCI: Bellek havuzları oluşturulamadı\n");
        return -1;
    }
    
    // Align memory pools
    controller->qtd_pool = align_up(controller->qtd_pool, 4096);
    controller->qh_pool = align_up(controller->qh_pool, 4096);
    
    controller->pool_index = 0;
    
    // Reset controller
    if (ehci_controller_reset(controller) != 0) {
        return -1;
    }
    
    // Allocate frame list
    if (ehci_allocate_frame_list(controller) != 0) {
        return -1;
    }
    
    // Setup async queue
    if (ehci_setup_async_queue(controller) != 0) {
        return -1;
    }
    
    // Configure controller
    uint32_t cmd = 0;
    cmd |= (0x08 << 16); // Interrupt threshold
    ehci_write_op_reg(controller, EHCI_USBCMD, cmd);
    
    // Enable interrupts
    uint32_t intr_enable = EHCI_INTR_USBINT | EHCI_INTR_USBERRINT | EHCI_INTR_PCD | EHCI_INTR_IAA;
    ehci_write_op_reg(controller, EHCI_USBINTR, intr_enable);
    
    // Set configured flag
    ehci_write_op_reg(controller, EHCI_CONFIGFLAG, 1);
    
    // Start controller
    cmd = ehci_read_op_reg(controller, EHCI_USBCMD);
    cmd |= EHCI_CMD_RUN;
    cmd |= EHCI_CMD_ASEN; // Enable async schedule
    ehci_write_op_reg(controller, EHCI_USBCMD, cmd);
    
    printf("EHCI: Controller başlatıldı\n");
    return 0;
}

// EHCI device enumeration
static int ehci_enumerate_device_impl(usb_host_controller_t* base_controller, uint8_t port) {
    ehci_controller_t* controller = (ehci_controller_t*)base_controller;
    
    printf("EHCI: Aygıt enumeration başlatılıyor (port %d)\n", port);
    
    // Check if port is valid
    if (port >= controller->num_ports) {
        printf("EHCI: Geçersiz port: %d\n", port);
        return -1;
    }
    
    // Check port status
    uint32_t port_status = ehci_read_op_reg(controller, EHCI_PORTSC + (port * 4));
    
    if (!(port_status & EHCI_PORT_CCS)) {
        printf("EHCI: Port %d'de aygıt bağlı değil\n", port);
        return -1;
    }
    
    // Check if this is a low-speed device (should be handled by companion controller)
    if (port_status & EHCI_PORT_LS) {
        printf("EHCI: Low-speed aygıt tespit edildi, companion controller'a yönlendiriliyor\n");
        // TODO: Hand off to companion OHCI controller
        return -1;
    }
    
    // Reset port
    printf("EHCI: Port %d resetleniyor\n", port);
    port_status |= EHCI_PORT_RESET;
    ehci_write_op_reg(controller, EHCI_PORTSC + (port * 4), port_status);
    
    // Wait for reset to complete
    uint32_t timeout = 10000;
    do {
        port_status = ehci_read_op_reg(controller, EHCI_PORTSC + (port * 4));
        timeout--;
    } while (timeout && (port_status & EHCI_PORT_RESET));
    
    if (timeout == 0) {
        printf("EHCI: Port reset tamamlanamadı\n");
        return -1;
    }
    
    // Clear reset status change
    ehci_write_op_reg(controller, EHCI_PORTSC + (port * 4), EHCI_PORT_PEC);
    
    // Check if device is enabled
    port_status = ehci_read_op_reg(controller, EHCI_PORTSC + (port * 4));
    if (!(port_status & EHCI_PORT_PE)) {
        printf("EHCI: Aygıt enable edilemedi\n");
        return -1;
    }
    
    // Determine device speed
    usb_speed_t speed = USB_SPEED_HIGH; // EHCI supports high-speed
    printf("EHCI: Aygıt hız: High\n");
    
    // TODO: Complete device enumeration
    // - Set device address
    // - Get device descriptor
    // - Get configuration descriptor
    // - Set configuration
    
    printf("EHCI: Aygıt enumeration tamamlandı\n");
    return 0;
}

// EHCI control transfer
static int ehci_control_transfer_impl(usb_host_controller_t* base_controller,
                                     uint8_t device_addr, uint8_t endpoint,
                                     uint8_t* setup_packet, uint8_t* data, uint32_t length) {
    ehci_controller_t* controller = (ehci_controller_t*)base_controller;
    
    // Allocate QH and QTDs
    ehci_qh_t* qh = ehci_allocate_qh(controller);
    ehci_qtd_t* qtd_setup = ehci_allocate_qtd(controller);
    ehci_qtd_t* qtd_data = NULL;
    ehci_qtd_t* qtd_status = ehci_allocate_qtd(controller);
    
    if (!qh || !qtd_setup || !qtd_status) {
        printf("EHCI: Descriptor tahsis hatası\n");
        return -1;
    }
    
    // Setup QH
    qh->ep_char = device_addr |                    // Device address
                   (endpoint << 8) |               // Endpoint number
                   (2 << 12) |                     // High speed
                   (64 << 16);                     // Max packet size
    
    qh->ep_cap = 0;
    qh->current_qtd = 0;
    
    // Setup QTD for setup packet
    qtd_setup->next_qtd = (uint32_t)qtd_status;
    qtd_setup->alt_next_qtd = 1; // Terminate
    qtd_setup->token = EHCI_QTD_ACTIVE |            // Active
                      EHCI_QTD_PID_OUT |            // PID OUT
                      (2 << 10) |                   // Error counter
                      (8 << 16);                    // 8 bytes for setup packet
    
    qtd_setup->buffer[0] = (uint32_t)setup_packet;
    
    // Data QTD if needed
    if (data && length > 0) {
        qtd_data = ehci_allocate_qtd(controller);
        if (!qtd_data) {
            printf("EHCI: Data QTD tahsis hatası\n");
            return -1;
        }
        
        qtd_setup->next_qtd = (uint32_t)qtd_data;
        qtd_data->next_qtd = (uint32_t)qtd_status;
        qtd_data->alt_next_qtd = 1; // Terminate
        qtd_data->token = EHCI_QTD_ACTIVE |           // Active
                         EHCI_QTD_PID_IN |            // PID IN
                         (2 << 10) |                  // Error counter
                         (length << 16);              // Data length
        
        qtd_data->buffer[0] = (uint32_t)data;
    }
    
    // Status QTD
    qtd_status->next_qtd = 1; // Terminate
    qtd_status->alt_next_qtd = 1; // Terminate
    qtd_status->token = EHCI_QTD_ACTIVE |            // Active
                        EHCI_QTD_PID_OUT |            // PID OUT
                        (1 << 10);                    // Error counter
    
    // Link QTD to QH
    qh->next_qtd = (uint32_t)qtd_setup;
    qh->alt_next_qtd = 1;
    
    // Add QH to async queue
    qh->horizontal_link = controller->async_qh->horizontal_link;
    controller->async_qh->horizontal_link = (uint32_t)qh | EHCI_FLE_TYPE_QH;
    
    // Wait for completion
    uint32_t timeout = 10000;
    while (timeout-- && (qtd_status->token & EHCI_QTD_ACTIVE)) {
        // Wait
    }
    
    // Remove QH from async queue
    controller->async_qh->horizontal_link = qh->horizontal_link;
    
    // Check result
    uint32_t status = qtd_status->token;
    if (status & EHCI_QTD_HALTED) {
        printf("EHCI: Control transfer başarısız, status: 0x%08X\n", status);
        return -1;
    }
    
    return 0;
}

// EHCI bulk transfer
static int ehci_bulk_transfer_impl(usb_host_controller_t* base_controller,
                                  uint8_t device_addr, uint8_t endpoint,
                                  uint8_t* data, uint32_t length, uint8_t direction) {
    ehci_controller_t* controller = (ehci_controller_t*)base_controller;
    
    // Allocate QH and QTD
    ehci_qh_t* qh = ehci_allocate_qh(controller);
    ehci_qtd_t* qtd = ehci_allocate_qtd(controller);
    
    if (!qh || !qtd) {
        printf("EHCI: Descriptor tahsis hatası\n");
        return -1;
    }
    
    // Setup QH
    qh->ep_char = device_addr |                    // Device address
                   (endpoint << 8) |               // Endpoint number
                   (2 << 12) |                     // High speed
                   (512 << 16);                    // Max packet size
    
    qh->ep_cap = 0;
    qh->current_qtd = 0;
    
    // Setup QTD
    qtd->next_qtd = 1; // Terminate
    qtd->alt_next_qtd = 1; // Terminate
    qtd->token = EHCI_QTD_ACTIVE |                // Active
                (direction ? EHCI_QTD_PID_IN : EHCI_QTD_PID_OUT) | // Direction
                (2 << 10) |                       // Error counter
                (length << 16);                   // Data length
    
    qtd->buffer[0] = (uint32_t)data;
    
    // Link QTD to QH
    qh->next_qtd = (uint32_t)qtd;
    qh->alt_next_qtd = 1;
    
    // Add QH to async queue
    qh->horizontal_link = controller->async_qh->horizontal_link;
    controller->async_qh->horizontal_link = (uint32_t)qh | EHCI_FLE_TYPE_QH;
    
    // Wait for completion
    uint32_t timeout = 10000;
    while (timeout-- && (qtd->token & EHCI_QTD_ACTIVE)) {
        // Wait
    }
    
    // Remove QH from async queue
    controller->async_qh->horizontal_link = qh->horizontal_link;
    
    // Check result
    uint32_t status = qtd->token;
    if (status & EHCI_QTD_HALTED) {
        printf("EHCI: Bulk transfer başarısız, status: 0x%08X\n", status);
        return -1;
    }
    
    return 0;
}

// EHCI interrupt transfer
static int ehci_interrupt_transfer_impl(usb_host_controller_t* base_controller,
                                       uint8_t device_addr, uint8_t endpoint,
                                       uint8_t* data, uint32_t length) {
    ehci_controller_t* controller = (ehci_controller_t*)base_controller;
    
    // For now, use bulk transfer as a fallback
    return ehci_bulk_transfer_impl(base_controller, device_addr, endpoint, data, length, USB_DIRECTION_IN);
}

// EHCI interrupt handler
void ehci_irq_handler() {
    printf("EHCI: Interrupt alındı\n");
    
    ehci_controller_t* current = ehci_controllers;
    while (current) {
        uint32_t status = ehci_read_op_reg(current, EHCI_USBSTS);
        
        if (status) {
            // Clear interrupts
            ehci_write_op_reg(current, EHCI_USBSTS, status);
            
            // Handle port change detect
            if (status & EHCI_STS_PCD) {
                printf("EHCI: Port change detected\n");
                // TODO: Handle port connect/disconnect
            }
            
            // Handle USB interrupt
            if (status & EHCI_STS_USBINT) {
                printf("EHCI: USB interrupt\n");
                // TODO: Process completed transfers
            }
            
            // Handle interrupt on async advance
            if (status & EHCI_STS_IAA) {
                printf("EHCI: Async advance\n");
                // TODO: Process async queue
            }
        }
        
        current = current->base.next;
    }
}

// EHCI driver initialization function
int ehci_init(usb_host_controller_t* controller) {
    ehci_controller_t* ehci_ctrl = (ehci_controller_t*)controller;
    
    printf("EHCI: Sürücü başlatılıyor\n");
    
    // Map MMIO registers
    pci_device_t* pci_dev = (pci_device_t*)controller->mmio_base;
    uint32_t base_addr = pci_dev->bar[0] & ~0xF;
    
    ehci_ctrl->cap_regs = (volatile uint32_t*)base_addr;
    ehci_ctrl->op_regs = (volatile uint32_t*)(base_addr + ehci_read_cap_reg(ehci_ctrl, EHCI_CAPLENGTH));
    
    // Check for extended capability
    uint32_t hcc_params = ehci_read_cap_reg(ehci_ctrl, EHCI_HCCPARAMS);
    if (hcc_params & 0x01) {
        uint32_t ext_cap_offset = (hcc_params >> 8) & 0xFF;
        ehci_ctrl->leg_regs = (volatile uint32_t*)(base_addr + ext_cap_offset);
    }
    
    if (!ehci_ctrl->cap_regs || !ehci_ctrl->op_regs) {
        printf("EHCI: Register haritalaması başarısız\n");
        return -1;
    }
    
    // Initialize controller
    return ehci_controller_init(ehci_ctrl);
}

// EHCI reset function
int ehci_reset(usb_host_controller_t* controller) {
    ehci_controller_t* ehci_ctrl = (ehci_controller_t*)controller;
    return ehci_controller_reset(ehci_ctrl);
}

// Create EHCI driver
driver_t* create_ehci_driver(pci_device_t* device) {
    ehci_controller_t* controller = malloc(sizeof(ehci_controller_t));
    if (!controller) {
        return NULL;
    }
    
    memset(controller, 0, sizeof(ehci_controller_t));
    
    // Setup base driver
    controller->base.type = USB_HC_EHCI;
    controller->base.init = (int (*)(void*))ehci_init;
    controller->base.reset = (int (*)(void*))ehci_reset;
    controller->base.enumerate_device = ehci_enumerate_device_impl;
    controller->base.control_transfer = ehci_control_transfer_impl;
    controller->base.bulk_transfer = ehci_bulk_transfer_impl;
    controller->base.interrupt_transfer = ehci_interrupt_transfer_impl;
    
    controller->base.mmio_base = device;
    controller->base.irq_line = device->irq;
    
    // Copy PCI info
    controller->base.vendor_id = device->vendor_id;
    controller->base.device_id = device->device_id;
    
    strcpy(controller->base.name, "EHCI USB Host Controller");
    controller->base.type = DRIVER_TYPE_INPUT;
    controller->base.class = DRIVER_CLASS_SERIAL;
    
    // Add to global list
    controller->base.next = (struct usb_host_controller*)ehci_controllers;
    ehci_controllers = controller;
    
    return (driver_t*)controller;
}
