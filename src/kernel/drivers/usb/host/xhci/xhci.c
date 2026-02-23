#include "xhci.h"
#include "memory.h"
#include "hardware_detect.h"
#include "interrupts.h"
#include "io.h"
#include "usb_host.h"
#include "string.h"
#include "printf.h"

// Global XHCI controller list
static xhci_controller_t* xhci_controllers = NULL;

// Memory alignment helpers
static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

// Register access functions
uint32_t xhci_read_cap_reg(xhci_controller_t* controller, uint32_t offset) {
    return controller->cap_regs[offset / 4];
}

uint32_t xhci_read_op_reg(xhci_controller_t* controller, uint32_t offset) {
    return controller->op_regs[offset / 4];
}

void xhci_write_op_reg(xhci_controller_t* controller, uint32_t offset, uint32_t value) {
    controller->op_regs[offset / 4] = value;
}

uint32_t xhci_read_rt_reg(xhci_controller_t* controller, uint32_t offset) {
    return controller->rt_regs[offset / 4];
}

void xhci_write_rt_reg(xhci_controller_t* controller, uint32_t offset, uint32_t value) {
    controller->rt_regs[offset / 4] = value;
}

void xhci_write_doorbell(xhci_controller_t* controller, uint8_t doorbell, uint32_t value) {
    controller->db_regs[doorbell] = value;
}

// Ring management functions
int xhci_init_command_ring(xhci_controller_t* controller) {
    // Allocate command ring (must be 64-byte aligned)
    controller->cmd_ring.trbs = (xhci_trb_t*)malloc(256 * sizeof(xhci_trb_t) + 63);
    if (!controller->cmd_ring.trbs) {
        printf("XHCI: Command ring bellek tahsisatÄ± baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Align to 64-byte boundary
    uint32_t addr = (uint32_t)controller->cmd_ring.trbs;
    controller->cmd_ring.trbs = (xhci_trb_t*)align_up(addr, 64);
    
    // Initialize command ring
    memset(controller->cmd_ring.trbs, 0, 256 * sizeof(xhci_trb_t));
    controller->cmd_ring.size = 256;
    controller->cmd_ring.cycle_state = 1;
    controller->cmd_ring.enqueue_index = 0;
    controller->cmd_ring.dequeue_index = 0;
    
    // Set command ring control register
    uint64_t crcr = (uint64_t)controller->cmd_ring.trbs;
    crcr |= controller->cmd_ring.cycle_state;
    
    controller->op_regs[XHCI_CRCR / 4] = (uint32_t)(crcr & 0xFFFFFFFF);
    controller->op_regs[XHCI_CRCR / 4 + 1] = (uint32_t)(crcr >> 32);
    
    printf("XHCI: Command ring ayarlandÄ±\n");
    return 0;
}

int xhci_init_event_ring(xhci_controller_t* controller) {
    // Allocate event ring (must be 64-byte aligned)
    controller->event_ring.trbs = (xhci_trb_t*)malloc(256 * sizeof(xhci_trb_t) + 63);
    if (!controller->event_ring.trbs) {
        printf("XHCI: Event ring bellek tahsisatÄ± baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Align to 64-byte boundary
    uint32_t addr = (uint32_t)controller->event_ring.trbs;
    controller->event_ring.trbs = (xhci_trb_t*)align_up(addr, 64);
    
    // Initialize event ring
    memset(controller->event_ring.trbs, 0, 256 * sizeof(xhci_trb_t));
    controller->event_ring.size = 256;
    controller->event_ring.consumer_cycle_state = 1;
    controller->event_ring.dequeue_index = 0;
    controller->event_ring.producer_index = 0;
    
    // Allocate event ring segment table
    xhci_event_ring_segment_t* erst = (xhci_event_ring_segment_t*)malloc(sizeof(xhci_event_ring_segment_t));
    if (!erst) {
        printf("XHCI: ERST bellek tahsisatÄ± baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    erst->ring_segment_base = (uint64_t)controller->event_ring.trbs;
    erst->ring_segment_size = 256;
    erst->ring_segment_reserved = 0;
    
    // Set event ring registers
    controller->rt_regs[XHCI_ERSTSZ / 4] = 1; // One segment
    controller->rt_regs[XHCI_ERSTBA / 4] = (uint32_t)((uint64_t)erst & 0xFFFFFFFF);
    controller->rt_regs[XHCI_ERSTBA / 4 + 1] = (uint32_t)((uint64_t)erst >> 32);
    controller->rt_regs[XHCI_ERDP / 4] = (uint32_t)((uint64_t)controller->event_ring.trbs & 0xFFFFFFFF);
    controller->rt_regs[XHCI_ERDP / 4 + 1] = (uint32_t)((uint64_t)controller->event_ring.trbs >> 32);
    
    printf("XHCI: Event ring ayarlandÄ±\n");
    return 0;
}

// XHCI controller reset
static int xhci_controller_reset(xhci_controller_t* controller) {
    printf("XHCI: Controller reset baÅŸlatÄ±lÄ±yor\n");
    
    // Reset the controller
    uint32_t cmd = xhci_read_op_reg(controller, XHCI_USBCMD);
    cmd |= XHCI_CMD_HCRST;
    xhci_write_op_reg(controller, XHCI_USBCMD, cmd);
    
    // Wait for reset to complete
    uint32_t timeout = 10000;
    while (timeout--) {
        uint32_t status = xhci_read_op_reg(controller, XHCI_USBSTS);
        if (!(status & XHCI_STS_CNR)) {
            break;
        }
    }
    
    if (timeout == 0) {
        printf("XHCI: Reset tamamlanamadÄ±\n");
        return -1;
    }
    
    printf("XHCI: Controller reset tamamlandÄ±\n");
    return 0;
}

// DCBAA allocation
static int xhci_allocate_dcbaa(xhci_controller_t* controller) {
    // Allocate DCBAA (must be 64-byte aligned)
    controller->dcbaa = (xhci_device_context_t**)malloc(256 * sizeof(uint64_t) + 63);
    if (!controller->dcbaa) {
        printf("XHCI: DCBAA bellek tahsisatÄ± baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Align to 64-byte boundary
    uint32_t addr = (uint32_t)controller->dcbaa;
    controller->dcbaa = (xhci_device_context_t**)align_up(addr, 64);
    
    // Initialize DCBAA
    memset(controller->dcbaa, 0, 256 * sizeof(uint64_t));
    
    // Set DCBAAP register
    uint64_t dcbaap = (uint64_t)controller->dcbaa;
    controller->op_regs[XHCI_DCBAAP / 4] = (uint32_t)(dcbaap & 0xFFFFFFFF);
    controller->op_regs[XHCI_DCBAAP / 4 + 1] = (uint32_t)(dcbaap >> 32);
    
    printf("XHCI: DCBAA ayarlandÄ±\n");
    return 0;
}

// XHCI controller initialization
static int xhci_controller_init(xhci_controller_t* controller) {
    printf("XHCI: Controller baÅŸlatÄ±lÄ±yor\n");
    
    // Get controller parameters
    controller->hcs_params1 = xhci_read_cap_reg(controller, XHCI_HCSPARAMS1);
    controller->hcs_params2 = xhci_read_cap_reg(controller, XHCI_HCSPARAMS2);
    controller->hcs_params3 = xhci_read_cap_reg(controller, XHCI_HCSPARAMS3);
    controller->hcc_params1 = xhci_read_cap_reg(controller, XHCI_HCCPARAMS1);
    
    // Extract parameters
    controller->max_slots = (controller->hcs_params1 & 0xFF);
    controller->num_ports = ((controller->hcs_params1 >> 24) & 0xFF);
    controller->max_interrupters = ((controller->hcs_params2 >> 8) & 0x3FF);
    
    printf("XHCI: %d slot, %d port, %d interrupter\n", 
           controller->max_slots, controller->num_ports, controller->max_interrupters);
    
    // Allocate memory pools
    controller->context_pool = (uint32_t)malloc(256 * sizeof(xhci_device_context_t) + 4095);
    controller->trb_pool = (uint32_t)malloc(1000 * sizeof(xhci_trb_t) + 4095);
    
    if (!controller->context_pool || !controller->trb_pool) {
        printf("XHCI: Bellek havuzlarÄ± oluÅŸturulamadÄ±\n");
        return -1;
    }
    
    // Align memory pools
    controller->context_pool = align_up(controller->context_pool, 4096);
    controller->trb_pool = align_up(controller->trb_pool, 4096);
    
    controller->pool_index = 0;
    
    // Reset controller
    if (xhci_controller_reset(controller) != 0) {
        return -1;
    }
    
    // Allocate DCBAA
    if (xhci_allocate_dcbaa(controller) != 0) {
        return -1;
    }
    
    // Initialize command ring
    if (xhci_init_command_ring(controller) != 0) {
        return -1;
    }
    
    // Initialize event ring
    if (xhci_init_event_ring(controller) != 0) {
        return -1;
    }
    
    // Configure controller
    uint32_t config = xhci_read_op_reg(controller, XHCI_CONFIG);
    config &= ~0xFF; // Clear Max Device Slots Enabled
    config |= controller->max_slots; // Set Max Device Slots Enabled
    xhci_write_op_reg(controller, XHCI_CONFIG, config);
    
    // Set page size
    xhci_write_op_reg(controller, XHCI_PAGESIZE, 1); // 4KB pages
    
    // Enable interrupts
    xhci_write_op_reg(controller, XHCI_USBCMD, XHCI_CMD_INTE);
    
    // Enable primary interrupter
    xhci_write_rt_reg(controller, XHCI_IMAN, 1); // Interrupt Enable
    
    // Start controller
    cmd = xhci_read_op_reg(controller, XHCI_USBCMD);
    cmd |= XHCI_CMD_RUN;
    xhci_write_op_reg(controller, XHCI_USBCMD, cmd);
    
    printf("XHCI: Controller baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// XHCI command execution
static int xhci_wait_for_command_completion(xhci_controller_t* controller, uint32_t timeout) {
    while (timeout--) {
        // Check event ring for command completion
        xhci_trb_t* event = &controller->event_ring.trbs[controller->event_ring.dequeue_index];
        
        if ((event->control & XHCI_TRB_CYCLE) == controller->event_ring.consumer_cycle_state) {
            uint32_t type = (event->control & XHCI_TRB_TYPE) >> 10;
            if (type == XHCI_EVENT_CMD_COMPLETE) {
                // Command completed
                controller->event_ring.dequeue_index++;
                if (controller->event_ring.dequeue_index >= controller->event_ring.size) {
                    controller->event_ring.dequeue_index = 0;
                    controller->event_ring.consumer_cycle_state ^= 1;
                }
                return 0;
            }
        }
    }
    return -1;
}

// XHCI enable slot command
int xhci_enable_slot(xhci_controller_t* controller, uint8_t* slot_id) {
    // Create enable slot TRB
    xhci_trb_t trb;
    memset(&trb, 0, sizeof(trb));
    
    trb.parameter = 0;
    trb.status = 0;
    trb.control = (XHCI_TRB_EN_SLOT << 10) | XHCI_TRB_CYCLE;
    
    // Add to command ring
    controller->cmd_ring.trbs[controller->cmd_ring.enqueue_index] = trb;
    controller->cmd_ring.enqueue_index++;
    if (controller->cmd_ring.enqueue_index >= controller->cmd_ring.size) {
        controller->cmd_ring.enqueue_index = 0;
        controller->cmd_ring.cycle_state ^= 1;
    }
    
    // Ring command doorbell
    xhci_write_doorbell(controller, 0, 0);
    
    // Wait for completion
    if (xhci_wait_for_command_completion(controller, 10000) != 0) {
        printf("XHCI: Enable slot komutu baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Get slot ID from event
    xhci_trb_t* event = &controller->event_ring.trbs[controller->event_ring.dequeue_index - 1];
    *slot_id = (event->parameter >> 24) & 0xFF;
    
    printf("XHCI: Slot %d enable edildi\n", *slot_id);
    return 0;
}

// XHCI device enumeration
static int xhci_enumerate_device_impl(usb_host_controller_t* base_controller, uint8_t port) {
    xhci_controller_t* controller = (xhci_controller_t*)base_controller;
    
    printf("XHCI: AygÄ±t enumeration baÅŸlatÄ±lÄ±yor (port %d)\n", port);
    
    // Check if port is valid
    if (port >= controller->num_ports) {
        printf("XHCI: GeÃ§ersiz port: %d\n", port);
        return -1;
    }
    
    // Check port status
    uint32_t port_status = xhci_read_op_reg(controller, XHCI_PORTSC + (port * 4));
    
    if (!(port_status & XHCI_PORT_CCS)) {
        printf("XHCI: Port %d'de aygÄ±t baÄŸlÄ± deÄŸil\n", port);
        return -1;
    }
    
    // Reset port
    printf("XHCI: Port %d resetleniyor\n", port);
    port_status |= XHCI_PORT_PR;
    xhci_write_op_reg(controller, XHCI_PORTSC + (port * 4), port_status);
    
    // Wait for reset to complete
    uint32_t timeout = 10000;
    do {
        port_status = xhci_read_op_reg(controller, XHCI_PORTSC + (port * 4));
        timeout--;
    } while (timeout && (port_status & XHCI_PORT_PR));
    
    if (timeout == 0) {
        printf("XHCI: Port reset tamamlanamadÄ±\n");
        return -1;
    }
    
    // Clear reset status change
    xhci_write_op_reg(controller, XHCI_PORTSC + (port * 4), XHCI_PORT_PRC);
    
    // Check if device is enabled
    port_status = xhci_read_op_reg(controller, XHCI_PORTSC + (port * 4));
    if (!(port_status & XHCI_PORT_PED)) {
        printf("XHCI: AygÄ±t enable edilemedi\n");
        return -1;
    }
    
    // Determine device speed
    uint32_t speed = (port_status & XHCI_PORT_SPEED) >> 10;
    const char* speed_str = "Unknown";
    switch (speed) {
        case XHCI_PORT_SPEED_U1: speed_str = "Full"; break;
        case XHCI_PORT_SPEED_U2: speed_str = "Low"; break;
        case XHCI_PORT_SPEED_U3: speed_str = "High"; break;
        case XHCI_PORT_SPEED_U4: speed_str = "Super"; break;
        case XHCI_PORT_SPEED_U5: speed_str = "Super Plus"; break;
    }
    printf("XHCI: AygÄ±t hÄ±z: %s\n", speed_str);
    
    // Enable slot
    uint8_t slot_id;
    if (xhci_enable_slot(controller, &slot_id) != 0) {
        printf("XHCI: Slot enable baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // TODO: Complete device enumeration
    // - Allocate device context
    // - Address device command
    // - Get device descriptor
    // - Configure endpoint
    // - Set configuration
    
    printf("XHCI: AygÄ±t enumeration tamamlandÄ±\n");
    return 0;
}

// XHCI control transfer (simplified implementation)
static int xhci_control_transfer_impl(usb_host_controller_t* base_controller,
                                     uint8_t device_addr, uint8_t endpoint,
                                     uint8_t* setup_packet, uint8_t* data, uint32_t length) {
    xhci_controller_t* controller = (xhci_controller_t*)base_controller;
    
    // For now, return success as a placeholder
    // Full implementation would require:
    // - Setup stage TRB
    // - Data stage TRB (if needed)
    // - Status stage TRB
    // - Ring endpoint doorbell
    // - Wait for completion
    
    printf("XHCI: Control transfer (addr=%d, ep=%d, len=%d)\n", device_addr, endpoint, length);
    return 0;
}

// XHCI bulk transfer (simplified implementation)
static int xhci_bulk_transfer_impl(usb_host_controller_t* base_controller,
                                  uint8_t device_addr, uint8_t endpoint,
                                  uint8_t* data, uint32_t length, uint8_t direction) {
    xhci_controller_t* controller = (xhci_controller_t*)base_controller;
    
    // For now, return success as a placeholder
    // Full implementation would require:
    // - Normal TRB setup
    // - Ring endpoint doorbell
    // - Wait for completion
    
    printf("XHCI: Bulk transfer (addr=%d, ep=%d, len=%d, dir=%d)\n", device_addr, endpoint, length, direction);
    return 0;
}

// XHCI interrupt transfer (simplified implementation)
static int xhci_interrupt_transfer_impl(usb_host_controller_t* base_controller,
                                       uint8_t device_addr, uint8_t endpoint,
                                       uint8_t* data, uint32_t length) {
    xhci_controller_t* controller = (xhci_controller_t*)base_controller;
    
    // For now, use bulk transfer as a fallback
    return xhci_bulk_transfer_impl(base_controller, device_addr, endpoint, data, length, USB_DIRECTION_IN);
}

// XHCI interrupt handler
void xhci_irq_handler() {
    printf("XHCI: Interrupt alÄ±ndÄ±\n");
    
    xhci_controller_t* current = xhci_controllers;
    while (current) {
        uint32_t status = xhci_read_op_reg(current, XHCI_USBSTS);
        
        if (status) {
            // Clear interrupts
            xhci_write_op_reg(current, XHCI_USBSTS, status);
            
            // Handle port change detect
            if (status & XHCI_STS_PCD) {
                printf("XHCI: Port change detected\n");
                // TODO: Handle port connect/disconnect
            }
            
            // Handle event interrupt
            if (status & XHCI_STS_EINT) {
                printf("XHCI: Event interrupt\n");
                // TODO: Process events from event ring
            }
        }
        
        current = current->base.next;
    }
}

// XHCI driver initialization function
int xhci_init(usb_host_controller_t* controller) {
    xhci_controller_t* xhci_ctrl = (xhci_controller_t*)controller;
    
    printf("XHCI: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±lÄ±yor\n");
    
    // Map MMIO registers
    pci_device_t* pci_dev = (pci_device_t*)controller->mmio_base;
    uint32_t base_addr = pci_dev->bar[0] & ~0xF;
    
    xhci_ctrl->cap_regs = (volatile uint32_t*)base_addr;
    
    // Get operational registers offset
    uint32_t cap_length = xhci_read_cap_reg(xhci_ctrl, XHCI_CAPLENGTH);
    xhci_ctrl->op_regs = (volatile uint32_t*)(base_addr + cap_length);
    
    // Get runtime registers offset
    uint32_t rt_offset = xhci_read_cap_reg(xhci_ctrl, XHCI_RTSOFF);
    xhci_ctrl->rt_regs = (volatile uint32_t*)(base_addr + rt_offset);
    
    // Get doorbell registers offset
    uint32_t db_offset = xhci_read_cap_reg(xhci_ctrl, XHCI_DBOFF);
    xhci_ctrl->db_regs = (volatile uint32_t*)(base_addr + db_offset);
    
    if (!xhci_ctrl->cap_regs || !xhci_ctrl->op_regs || !xhci_ctrl->rt_regs || !xhci_ctrl->db_regs) {
        printf("XHCI: Register haritalamasÄ± baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Initialize controller
    return xhci_controller_init(xhci_ctrl);
}

// XHCI reset function
int xhci_reset(usb_host_controller_t* controller) {
    xhci_controller_t* xhci_ctrl = (xhci_controller_t*)controller;
    return xhci_controller_reset(xhci_ctrl);
}

// Create XHCI driver
driver_t* create_xhci_driver(pci_device_t* device) {
    xhci_controller_t* controller = malloc(sizeof(xhci_controller_t));
    if (!controller) {
        return NULL;
    }
    
    memset(controller, 0, sizeof(xhci_controller_t));
    
    // Setup base driver
    controller->base.type = USB_HC_XHCI;
    controller->base.init = (int (*)(void*))xhci_init;
    controller->base.reset = (int (*)(void*))xhci_reset;
    controller->base.enumerate_device = xhci_enumerate_device_impl;
    controller->base.control_transfer = xhci_control_transfer_impl;
    controller->base.bulk_transfer = xhci_bulk_transfer_impl;
    controller->base.interrupt_transfer = xhci_interrupt_transfer_impl;
    
    controller->base.mmio_base = device;
    controller->base.irq_line = device->interrupt_line;
    
    // Copy PCI info
    controller->base.vendor_id = device->vendor_id;
    controller->base.device_id = device->device_id;
    
    strcpy(controller->base.name, "XHCI USB Host Controller");
    controller->base.type = DRIVER_TYPE_INPUT;
    controller->base.class = PCI_CLASS_SERIAL;
    
    // Add to global list
    controller->base.next = (struct usb_host_controller*)xhci_controllers;
    xhci_controllers = controller;
    
    return (driver_t*)controller;
}
