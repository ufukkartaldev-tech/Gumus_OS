#ifndef EHCI_H
#define EHCI_H

#include "usb_host.h"

// EHCI Capability Registers
#define EHCI_CAPLENGTH            0x00
#define EHCI_HCIVERSION           0x02
#define EHCI_HCSPARAMS            0x04
#define EHCI_HCCPARAMS            0x08

// EHCI Operational Registers
#define EHCI_USBCMD               0x00
#define EHCI_USBSTS               0x04
#define EHCI_USBINTR              0x08
#define EHCI_FRINDEX              0x0C
#define EHCI_CTRLDSSEGMENT        0x10
#define EHCI_PERIODICLISTBASE     0x14
#define EHCI_ASYNCLISTADDR        0x18
#define EHCI_CONFIGFLAG           0x40
#define EHCI_PORTSC               0x44

// EHCI Extended Capability Registers
#define EHCI_USBLEGSUP            0x00
#define EHCI_USBLEGCTLSTS         0x04

// EHCI Command Register bits
#define EHCI_CMD_RUN              (1 << 0)   // Run/Stop
#define EHCI_CMD_HCRESET          (1 << 1)   // Host Controller Reset
#define EHCI_CMD_PSEN             (1 << 2)   // Periodic Schedule Enable
#define EHCI_CMD_ASEN             (1 << 3)   // Asynchronous Schedule Enable
#define EHCI_CMD_IAAD             (1 << 5)   // Interrupt on Async Advance Doorbell
#define EHCI_CMD_LHCR             (1 << 6)   // Light Host Controller Reset
#define EHCI_CMD_ITC              (3 << 16)  // Interrupt Threshold Control

// EHCI Status Register bits
#define EHCI_STS_USBINT           (1 << 0)   // USB Interrupt
#define EHCI_STS_USBERRINT        (1 << 1)   // USB Error Interrupt
#define EHCI_STS_PCD              (1 << 2)   // Port Change Detect
#define EHCI_STS_FLR              (1 << 3)   // Frame List Rollover
#define EHCI_STS_HSE              (1 << 4)   // Host System Error
#define EHCI_STS_IAA              (1 << 5)   // Interrupt on Async Advance
#define EHCI_STS_HCHalted         (1 << 12)  // Host Controller Halted
#define EHCI_STS_RECL             (1 << 13)  // Reclamation
#define EHCI_STS_PSS              (1 << 14)  // Periodic Schedule Status
#define EHCI_STS_ASS              (1 << 15)  // Asynchronous Schedule Status

// EHCI Interrupt Enable bits
#define EHCI_INTR_USBINT          (1 << 0)   // USB Interrupt
#define EHCI_INTR_USBERRINT       (1 << 1)   // USB Error Interrupt
#define EHCI_INTR_PCD             (1 << 2)   // Port Change Detect
#define EHCI_INTR_FLR             (1 << 3)   // Frame List Rollover
#define EHCI_INTR_HSE             (1 << 4)   // Host System Error
#define EHCI_INTR_IAA             (1 << 5)   // Interrupt on Async Advance

// EHCI Port Status and Control Register bits
#define EHCI_PORT_CCS             (1 << 0)   // Current Connect Status
#define EHCI_PORT_CSC             (1 << 1)   // Connect Status Change
#define EHCI_PORT_PE              (1 << 2)   // Port Enable
#define EHCI_PORT_PEC             (1 << 3)   // Port Enable/Disable Change
#define EHCI_PORT_OCA             (1 << 4)   // Over-current Active
#define EHCI_PORT_OCC             (1 << 5)   // Over-current Change
#define EHCI_PORT_FPR             (1 << 6)   // Force Port Resume
#define EHCI_PORT_SUSPEND         (1 << 7)   // Suspend
#define EHCI_PORT_RESET           (1 << 8)   // Reset
#define EHCI_PORT_LINE_STS        (3 << 10)  // Line Status
#define EHCI_PORT_POWER           (1 << 12)  // Port Power
#define EHCI_PORT_OWNER           (1 << 13)  // Port Owner
#define EHCI_PORT_LS              (1 << 14)  // Low Speed Device Attached
#define EHCI_PORT_PP              (1 << 16)  // Port Power Control

// EHCI QTD (Queue Element Transfer Descriptor)
typedef struct ehci_qtd {
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
    uint32_t extended_buffer[5];
} __attribute__((packed)) ehci_qtd_t;

// EHCI QTD Token bits
#define EHCI_QTD_STATUS           0xFF
#define EHCI_QTD_PID              (3 << 8)   // PID Code
#define EHCI_QTD_CPAGE            (7 << 12)  // Current Page
#define EHCI_QTD_CERR             (3 << 10)  // Error Counter
#define EHCI_QTD_ACTIVE           (1 << 7)   // Active
#define EHCI_QTD_HALTED           (1 << 6)   // Halted
#define EHCI_QTD_DATATOGGLE       (1 << 31)  // Data Toggle
#define EHCI_QTD_TOTAL_BYTES      0x7FFF0000 // Total Bytes to Transfer

// EHCI QH (Queue Head)
typedef struct ehci_qh {
    uint32_t horizontal_link;
    uint32_t ep_char;
    uint32_t ep_cap;
    uint32_t current_qtd;
    uint32_t next_qtd;
    uint32_t alt_next_qtd;
    uint32_t token;
    uint32_t buffer[5];
} __attribute__((packed)) ehci_qh_t;

// EHCI QH Endpoint Characteristics bits
#define EHCI_QH_DEV_ADDR         0x000000FF  // Device Address
#define EHCI_QH_EP_NUM            0x00000F00  // Endpoint Number
#define EHCI_QH_EPS               0x00003000  // Endpoint Speed
#define EHCI_QH_DTC               0x00004000  // Data Toggle Control
#define EHCI_QH_HRL               0x00008000  // Head of Reclamation List
#define EHCI_QH_MPL               0x07FF0000  // Maximum Packet Length
#define EHCI_QH_CTRL_EP           0x08000000  // Control Endpoint
#define EHCI_QH_NAK_RL            0xF0000000  // NAK Reload

// EHCI QH Endpoint Capabilities bits
#define EHCI_QH_UFRAME_CMASK      0x000000FF  // Micro-frame Schedule Control Mask
#define EHCI_QH_UFRAME_SMASK      0x0000FF00  // Micro-frame Schedule Split Mask
#define EHCI_QH_HUB_ADDR          0x007F0000  // Hub Address
#define EHCI_QH_PORT_NUM          0x7F000000  // Port Number
#define EHCI_QH_MULT              0xC0000000  // High Bandwidth Multiplier

// EHCI Frame List Entry
typedef struct ehci_fle {
    uint32_t link;
} __attribute__((packed)) ehci_fle_t;

// EHCI Frame List Link bits
#define EHCI_FLE_TERMINATE        0x00000001  // Terminate
#define EHCI_FLE_TYPE             0x00000006  // Type
#define EHCI_FLE_TYPE_ITD         0x00000000  // Isochronous TD
#define EHCI_FLE_TYPE_QH          0x00000002  // Queue Head
#define EHCI_FLE_TYPE_SITD        0x00000004  // Split Isochronous TD
#define EHCI_FLE_TYPE_FSTN        0x00000006  // Frame Spanning Traversal Node

// EHCI Controller Structure
typedef struct {
    usb_host_controller_t base;
    
    // EHCI specific registers
    volatile uint32_t* cap_regs;      // Capability registers
    volatile uint32_t* op_regs;       // Operational registers
    volatile uint32_t* leg_regs;      // Legacy support registers
    
    // Memory structures
    ehci_fle_t* frame_list;           // Frame list
    ehci_qh_t* async_qh;              // Asynchronous queue head
    ehci_qh_t* periodic_qh;           // Periodic queue head
    
    // Memory management
    uint32_t qtd_pool;
    uint32_t qh_pool;
    uint32_t pool_index;
    
    // Controller properties
    uint32_t num_ports;
    uint32_t frame_list_size;
    uint32_t hcs_params;
    uint32_t hcc_params;
    
    // State
    uint8_t companion_controllers;
    uint8_t companion_ports;
} ehci_controller_t;

// EHCI Function Prototypes
int ehci_init(usb_host_controller_t* controller);
int ehci_reset(usb_host_controller_t* controller);
int ehci_enumerate_device(usb_host_controller_t* controller, uint8_t port);
int ehci_control_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                        uint8_t endpoint, uint8_t* setup_packet, uint8_t* data, uint32_t length);
int ehci_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                      uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction);
int ehci_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                          uint8_t endpoint, uint8_t* data, uint32_t length);
void ehci_irq_handler();

// Memory management functions
ehci_qtd_t* ehci_allocate_qtd(ehci_controller_t* controller);
ehci_qh_t* ehci_allocate_qh(ehci_controller_t* controller);
void ehci_free_qtd(ehci_controller_t* controller, ehci_qtd_t* qtd);
void ehci_free_qh(ehci_controller_t* controller, ehci_qh_t* qh);

// Register access functions
uint32_t ehci_read_cap_reg(ehci_controller_t* controller, uint32_t offset);
uint32_t ehci_read_op_reg(ehci_controller_t* controller, uint32_t offset);
void ehci_write_op_reg(ehci_controller_t* controller, uint32_t offset, uint32_t value);
uint32_t ehci_read_leg_reg(ehci_controller_t* controller, uint32_t offset);
void ehci_write_leg_reg(ehci_controller_t* controller, uint32_t offset, uint32_t value);

#endif
