#ifndef OHCI_H
#define OHCI_H

#include "usb_host.h"

// OHCI Registers
#define OHCI_REVISION               0x00
#define OHCI_CONTROL                0x04
#define OHCI_CMD_STATUS             0x08
#define OHCI_INTERRUPT_STATUS       0x0C
#define OHCI_INTERRUPT_ENABLE       0x10
#define OHCI_INTERRUPT_DISABLE      0x14
#define OHCI_HCCA                 0x18
#define OHCI_PERIODIC_CURRENT_ED   0x1C
#define OHCI_CONTROL_HEAD_ED       0x20
#define OHCI_CONTROL_CURRENT_ED    0x24
#define OHCI_BULK_HEAD_ED          0x28
#define OHCI_BULK_CURRENT_ED       0x2C
#define OHCI_DONE_HEAD             0x30
#define OHCI_FM_INTERVAL           0x34
#define OHCI_FM_REMAINING          0x38
#define OHCI_FM_NUMBER             0x3C
#define OHCI_PERIODIC_START        0x40
#define OHCI_LS_THRESHOLD          0x44
#define OHCI_RH_DESCRIPTOR_A       0x48
#define OHCI_RH_DESCRIPTOR_B       0x4C
#define OHCI_RH_STATUS             0x50
#define OHCI_RH_PORT_STATUS        0x54

// OHCI Control bits
#define OHCI_CTRL_CBSR       (1 << 0)  // Control Bulk Service Ratio
#define OHCI_CTRL_PLE         (1 << 1)  // Periodic List Enable
#define OHCI_CTRL_IE          (1 << 2)  // Isochronous Enable
#define OHCI_CTRL_CLE         (1 << 3)  // Control List Enable
#define OHCI_CTRL_BLE         (1 << 4)  // Bulk List Enable
#define OHCI_CTRL_HCFS        (3 << 6)  // Host Controller Functional State
#define OHCI_CTRL_IR          (1 << 8)  // Interrupt Routing
#define OHCI_CTRL_RWC         (1 << 9)  // Remote Wakeup Connected
#define OHCI_CTRL_RWE         (1 << 10) // Remote Wakeup Enable

// OHCI Functional States
#define OHCI_STATE_RESET      0x00
#define OHCI_STATE_RESUME     0x01
#define OHCI_STATE_OPERATIONAL 0x02
#define OHCI_STATE_SUSPEND    0x03

// OHCI Command Status bits
#define OHCI_CMD_HCR         (1 << 0)  // Host Controller Reset
#define OHCI_CMD_CLF         (1 << 1)  // Control List Filled
#define OHCI_CMD_BLF         (1 << 2)  // Bulk List Filled
#define OHCI_CMD_OCR         (1 << 3)  // Ownership Change Request

// OHCI Interrupt bits
#define OHCI_INTR_SO         (1 << 0)  // Scheduling Overrun
#define OHCI_INTR_WDH        (1 << 1)  // Writeback Done Head
#define OHCI_INTR_SF         (1 << 2)  // Start of Frame
#define OHCI_INTR_RD         (1 << 3)  // Resume Detected
#define OHCI_INTR_UE         (1 << 4)  // Unrecoverable Error
#define OHCI_INTR_FNO        (1 << 5)  // Frame Number Overflow
#define OHCI_INTR_RHSC       (1 << 6)  // Root Hub Status Change
#define OHCI_INTR_OC         (1 << 30) // Ownership Change
#define OHCI_INTR_MIE        (1 << 31) // Master Interrupt Enable

// OHCI Root Hub bits
#define OHCI_RH_LPS         (1 << 0)  // Local Power Status
#define OHCI_RH_OCI         (1 << 1)  // Over Current Indicator
#define OHCI_RH_DRWE        (1 << 15) // Device Remote Wakeup Enable
#define OHCI_RH_LPSC        (1 << 16) // Local Power Status Change
#define OHCI_RH_OCIC        (1 << 17) // Over Current Indicator Change
#define OHCI_RH_CRWE        (1 << 31) // Clear Remote Wakeup Enable

// OHCI Port Status bits
#define OHCI_PORT_CCS        (1 << 0)  // Current Connect Status
#define OHCI_PORT_PES        (1 << 1)  // Port Enable Status
#define OHCI_PORT_PSS        (1 << 2)  // Port Suspend Status
#define OHCI_PORT_POCI       (1 << 3)  // Port Over Current Indicator
#define OHCI_PORT_PRS        (1 << 4)  // Port Reset Status
#define OHCI_PORT_PPS        (1 << 8)  // Port Power Status
#define OHCI_PORT_LSDA       (1 << 9)  // Low Speed Device Attached
#define OHCI_PORT_CSC        (1 << 16) // Connect Status Change
#define OHCI_PORT_PESC        (1 << 17) // Port Enable Status Change
#define OHCI_PORT_PSSC       (1 << 18) // Port Suspend Status Change
#define OHCI_PORT_OCIC       (1 << 19) // Over Current Indicator Change
#define OHCI_PORT_PRSC       (1 << 20) // Port Reset Status Change

// OHCI HCCA (Host Controller Communication Area)
typedef struct {
    uint32_t interrupt_table[32];
    uint16_t frame_number;
    uint16_t pad1;
    uint32_t done_head;
    uint32_t reserved[29];
} __attribute__((packed)) ohci_hcca_t;

// OHCI Endpoint Descriptor
typedef struct ohci_ed {
    uint32_t control;
    uint32_t tail_td;
    uint32_t head_td;
    uint32_t next_ed;
} __attribute__((packed)) ohci_ed_t;

// OHCI Transfer Descriptor
typedef struct ohci_td {
    uint32_t control;
    uint32_t cbp;          // Current Buffer Pointer
    uint32_t next_td;
    uint32_t be;           // Buffer End
} __attribute__((packed)) ohci_td_t;

// OHCI Transfer Descriptor Control bits
#define OHCI_TD_CC          0x0000000F  // Condition Code
#define OHCI_TD_CC_NOERROR  0x00
#define OHCI_TD_CC_CRC      0x01
#define OHCI_TD_CC_BITSTUFF 0x02
#define OHCI_TD_CC_TOGGLE   0x03
#define OHCI_TD_CC_STALL    0x04
#define OHCI_TD_DEVICENOTRESP 0x05
#define OHCI_TD_PIDCHECKFAIL 0x06
#define OHCI_TD_UNEXPECTEDPID 0x07
#define OHCI_TD_DATAOVERRUN 0x08
#define OHCI_TD_DATAUNDERRUN 0x09
#define OHCI_TD_BUFFEROVERRUN 0x0C
#define OHCI_TD_BUFFERUNDERRUN 0x0D
#define OHCI_TD_NOTACCESSED 0x0E

#define OHCI_TD_DI          (1 << 4)    // Delay Interrupt
#define OHCI_TD_T           (1 << 5)    // Data Toggle
#define OHCI_TD_T0          (0 << 5)    // Toggle 0
#define OHCI_TD_T1          (1 << 5)    // Toggle 1
#define OHCI_TD_EC          (1 << 6)    // Error Count
#define OHCI_TD_R           (1 << 7)    // Round Robin

// OHCI Endpoint Descriptor Control bits
#define OHCI_ED_FA          0x000000FF  // Function Address
#define OHCI_ED_EN          0x00000700  // Endpoint Number
#define OHCI_ED_DIR         0x00001800  // Direction
#define OHCI_ED_DIR_OUT     0x00000000
#define OHCI_ED_DIR_IN      0x00001000
#define OHCI_ED_SPEED       0x00002000  // Speed
#define OHCI_ED_SKIP        0x00004000  // Skip
#define OHCI_ED_FORMAT      0x00008000  // Format
#define OHCI_ED_MPS         0x07FF0000  // Maximum Packet Size

// OHCI Controller Yapısı
typedef struct {
    usb_host_controller_t base;
    
    // OHCI specific registers
    volatile uint32_t* registers;
    
    // HCCA
    ohci_hcca_t* hcca;
    
    // Memory management
    uint32_t ed_pool;
    uint32_t td_pool;
    uint32_t pool_index;
    
    // Controller state
    uint8_t num_ports;
    uint32_t revision;
} ohci_controller_t;

// OHCI Fonksiyon Prototipleri
int ohci_init(usb_host_controller_t* controller);
int ohci_reset(usb_host_controller_t* controller);
int ohci_enumerate_device(usb_host_controller_t* controller, uint8_t port);
int ohci_control_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                        uint8_t endpoint, uint8_t* setup_packet, uint8_t* data, uint32_t length);
int ohci_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                      uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction);
int ohci_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                          uint8_t endpoint, uint8_t* data, uint32_t length);
void ohci_irq_handler();

#endif
