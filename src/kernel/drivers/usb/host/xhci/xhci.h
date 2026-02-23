#ifndef XHCI_H
#define XHCI_H

#include "usb_host.h"

// XHCI Capability Registers
#define XHCI_CAPLENGTH            0x00
#define XHCI_HCIVERSION           0x02
#define XHCI_HCSPARAMS1           0x04
#define XHCI_HCSPARAMS2           0x08
#define XHCI_HCSPARAMS3           0x0C
#define XHCI_HCCPARAMS1           0x10
#define XHCI_DBOFF                0x14
#define XHCI_RTSOFF               0x18

// XHCI Operational Registers
#define XHCI_USBCMD               0x00
#define XHCI_USBSTS               0x04
#define XHCI_PAGESIZE             0x08
#define XHCI_DNCTRL               0x14
#define XHCI_CRCR                 0x18
#define XHCI_DCBAAP               0x30
#define XHCI_CONFIG               0x38
#define XHCI_PORTSC               0x40

// XHCI Runtime Registers
#define XHCI_MFINDEX              0x00
#define XHCI_IMAN                 0x20
#define XHCI_IMOD                 0x24
#define XHCI_ERSTSZ               0x28
#define XHCI_ERSTBA               0x30
#define XHCI_ERDP                 0x38

// XHCI Doorbell Registers
#define XHCI_DOORBELL             0x00

// XHCI Command Register bits
#define XHCI_CMD_RUN              (1 << 0)   // Run/Stop
#define XHCI_CMD_HCRST            (1 << 1)   // Host Controller Reset
#define XHCI_CMD_INTE             (1 << 2)   // Interrupter Enable
#define XHCI_CMD_HSEE             (1 << 3)   // Host System Error Enable
#define XHCI_CMD_EWE              (1 << 4)   // Enable Wrap Event
#define XHCI_CMD_CRS              (1 << 5)   // Controller Reset State

// XHCI Status Register bits
#define XHCI_STS_HCH              (1 << 0)   // Host Controller Halted
#define XHCI_STS_HSE              (1 << 2)   // Host System Error
#define XHCI_STS_EINT             (1 << 3)   // Event Interrupt
#define XHCI_STS_PCD              (1 << 4)   // Port Change Detect
#define XHCI_STS_SSS              (1 << 7)   // Save State Status
#define XHCI_STS_RSS              (1 << 8)   // Restore State Status
#define XHCI_STS_SRE              (1 << 10)  // Save/Restore Error
#define XHCI_STS_CNR              (1 << 11)  // Controller Not Ready
#define XHCI_STS_HCE              (1 << 12)  // Host Controller Error

// XHCI Port Status and Control Register bits
#define XHCI_PORT_CCS             (1 << 0)   // Current Connect Status
#define XHCI_PORT_PED             (1 << 1)   // Port Enabled/Disabled
#define XHCI_PORT_OCA             (1 << 3)   // Over-current Active
#define XHCI_PORT_PR               (1 << 4)   // Port Reset
#define XHCI_PORT_PLS             (0xF << 5)  // Port Link State
#define XHCI_PORT_PP              (1 << 9)   // Port Power
#define XHCI_PORT_SPEED           (0xF << 10) // Port Speed
#define XHCI_PORT_PIC             (0x3 << 14) // Port Indicator Control
#define XHCI_PORT_LWS             (1 << 16)  // Port Link State Write Strobe
#define XHCI_PORT_CSC             (1 << 17)  // Connect Status Change
#define XHCI_PORT_PEC             (1 << 18)  // Port Enabled/Disabled Change
#define XHCI_PORT_WRC             (1 << 19)  // Warm Port Reset Change
#define XHCI_PORT_OCC             (1 << 20)  // Over-current Change
#define XHCI_PORT_PRC             (1 << 21)  // Port Reset Change
#define XHCI_PORT_PLC             (1 << 22)  // Port Link State Change
#define XHCI_PORT_CEC             (1 << 23)  // Port Config Error Change

// XHCI Port Speed Values
#define XHCI_PORT_SPEED_U1        1           // Full Speed
#define XHCI_PORT_SPEED_U2        2           // Low Speed
#define XHCI_PORT_SPEED_U3        3           // High Speed
#define XHCI_PORT_SPEED_U4        4           // Super Speed
#define XHCI_PORT_SPEED_U5        5           // Super Speed Plus

// XHCI TRB (Transfer Request Block) Types
#define XHCI_TRB_NORMAL           1
#define XHCI_TRB_SETUP_STAGE      2
#define XHCI_TRB_DATA_STAGE       3
#define XHCI_TRB_STATUS_STAGE     4
#define XHCI_TRB_ISOCH            5
#define XHCI_TRB_LINK             6
#define XHCI_TRB_EVENT_DATA       7
#define XHCI_TRB_NO_OP            8
#define XHCI_TRB_EN_SLOT          9
#define XHCI_TRB_DIS_SLOT         10
#define XHCI_TRB_ADDRESS_DEV      11
#define XHCI_TRB_CONFIG_EP        12
#define XHCI_TRB_EVAL_CONTEXT     13
#define XHCI_TRB_RESET_EP         14
#define XHCI_TRB_STOP_EP          15
#define XHCI_TRB_SET_TR_DEQ       16
#define XHCI_TRB_RESET_DEV        17
#define XHCI_TRB_FORCE_EVENT      18
#define XHCI_TRB_NEG_BANDWIDTH    19
#define XHCI_TRB_SET_LATENCY      20
#define XHCI_TRB_GET_PORT_BAND    21
#define XHCI_TRB_FORCE_HEADER     22
#define XHCI_TRB_NO_OP_CMD        23

// XHCI Event TRB Types
#define XHCI_EVENT_TRANSFER       32
#define XHCI_EVENT_CMD_COMPLETE   33
#define XHCI_EVENT_PORT_STATUS    34
#define XHCI_EVENT_BANDWIDTH_REQ  35
#define XHCI_EVENT_DOORBELL       36
#define XHCI_EVENT_HOST_CTRL      37
#define XHCI_EVENT_DEVICE_NOTIFY  38
#define XHCI_EVENT_MFINDEX_WRAP   39

// XHCI TRB Structure
typedef struct {
    uint64_t parameter;
    uint32_t status;
    uint32_t control;
} __attribute__((packed)) xhci_trb_t;

// XHCI TRB Control bits
#define XHCI_TRB_TYPE             0x3F00
#define XHCI_TRB_CYCLE            (1 << 0)
#define XHCI_TRB_ENT              (1 << 1)
#define XHCI_TRB_ISP              (1 << 2)
#define XHCI_TRB_NO_SOP           (1 << 3)
#define XHCI_TRB_CHAIN            (1 << 4)
#define XHCI_TRB_IOC              (1 << 5)
#define XHCI_TRB_IDT              (1 << 6)
#define XHCI_TRB_TBC              (0x3 << 7)
#define XHCI_TRB_TLR              (0xF << 9)
#define XHCI_TRB_BEI              (1 << 15)
#define XHCI_TRB_DIR              (1 << 16)

// XHCI Slot Context
typedef struct {
    uint32_t drop_flags;
    uint32_t add_flags;
    uint32_t route;
    uint32_t sctx1;
    uint32_t sctx2;
    uint32_t reserved[3];
} __attribute__((packed)) xhci_slot_context_t;

// XHCI Endpoint Context
typedef struct {
    uint32_t ep_ctx1;
    uint32_t ep_ctx2;
    uint64_t tr_dequeue_ptr;
    uint32_t ep_ctx4;
    uint32_t reserved[3];
} __attribute__((packed)) xhci_ep_context_t;

// XHCI Input Control Context
typedef struct {
    uint32_t drop_context_flags;
    uint32_t add_context_flags;
    uint32_t reserved[6];
} __attribute__((packed)) xhci_input_control_context_t;

// XHCI Device Context
typedef struct {
    xhci_slot_context_t slot;
    xhci_ep_context_t ep[31]; // 31 endpoints (0-30)
} __attribute__((packed)) xhci_device_context_t;

// XHCI Input Context
typedef struct {
    xhci_input_control_context_t control;
    xhci_slot_context_t slot;
    xhci_ep_context_t ep[31]; // 31 endpoints (0-30)
} __attribute__((packed)) xhci_input_context_t;

// XHCI Event Ring Segment
typedef struct {
    uint64_t ring_segment_base;
    uint32_t ring_segment_size;
    uint32_t ring_segment_reserved;
} __attribute__((packed)) xhci_event_ring_segment_t;

// XHCI Event Ring
typedef struct {
    xhci_trb_t* trbs;
    uint32_t size;
    uint32_t consumer_cycle_state;
    uint32_t dequeue_index;
    uint32_t producer_index;
} xhci_event_ring_t;

// XHCI Command Ring
typedef struct {
    xhci_trb_t* trbs;
    uint32_t size;
    uint32_t cycle_state;
    uint32_t enqueue_index;
    uint32_t dequeue_index;
} xhci_command_ring_t;

// XHCI Transfer Ring
typedef struct {
    xhci_trb_t* trbs;
    uint32_t size;
    uint32_t cycle_state;
    uint32_t enqueue_index;
    uint32_t dequeue_index;
} xhci_transfer_ring_t;

// XHCI Controller Structure
typedef struct {
    usb_host_controller_t base;
    
    // XHCI specific registers
    volatile uint32_t* cap_regs;      // Capability registers
    volatile uint32_t* op_regs;       // Operational registers
    volatile uint32_t* rt_regs;       // Runtime registers
    volatile uint32_t* db_regs;       // Doorbell registers
    
    // Memory structures
    xhci_device_context_t** dcbaa;    // Device Context Base Address Array
    xhci_command_ring_t cmd_ring;     // Command ring
    xhci_event_ring_t event_ring;     // Event ring
    xhci_transfer_ring_t** transfer_rings; // Transfer rings
    
    // Scratchpad buffers
    void** scratchpad_buffers;
    uint32_t scratchpad_count;
    
    // Memory management
    uint32_t context_pool;
    uint32_t trb_pool;
    uint32_t pool_index;
    
    // Controller properties
    uint32_t num_ports;
    uint32_t max_slots;
    uint32_t max_interrupters;
    uint32_t hcs_params1;
    uint32_t hcs_params2;
    uint32_t hcs_params3;
    uint32_t hcc_params1;
    
    // State
    uint8_t* device_contexts[256];   // Device contexts for each slot
} xhci_controller_t;

// XHCI Function Prototypes
int xhci_init(usb_host_controller_t* controller);
int xhci_reset(usb_host_controller_t* controller);
int xhci_enumerate_device(usb_host_controller_t* controller, uint8_t port);
int xhci_control_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                        uint8_t endpoint, uint8_t* setup_packet, uint8_t* data, uint32_t length);
int xhci_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                      uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction);
int xhci_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                          uint8_t endpoint, uint8_t* data, uint32_t length);
void xhci_irq_handler();

// Command functions
int xhci_enable_slot(xhci_controller_t* controller, uint8_t* slot_id);
int xhci_disable_slot(xhci_controller_t* controller, uint8_t slot_id);
int xhci_address_device(xhci_controller_t* controller, uint8_t slot_id, uint64_t input_ctx_ptr);
int xhci_configure_endpoint(xhci_controller_t* controller, uint8_t slot_id, uint64_t input_ctx_ptr);
int xhci_evaluate_context(xhci_controller_t* controller, uint8_t slot_id, uint64_t input_ctx_ptr);
int xhci_reset_endpoint(xhci_controller_t* controller, uint8_t slot_id, uint8_t endpoint);
int xhci_stop_endpoint(xhci_controller_t* controller, uint8_t slot_id, uint8_t endpoint);
int xhci_set_tr_dequeue(xhci_controller_t* controller, uint8_t slot_id, uint8_t endpoint, uint64_t dequeue_ptr);
int xhci_reset_device(xhci_controller_t* controller, uint8_t slot_id);

// Ring management functions
int xhci_init_command_ring(xhci_controller_t* controller);
int xhci_init_event_ring(xhci_controller_t* controller);
int xhci_init_transfer_ring(xhci_controller_t* controller, uint8_t slot_id, uint8_t endpoint);
void xhci_ring_enqueue(xhci_trb_t* ring, uint32_t* index, uint32_t size, uint32_t* cycle_state, xhci_trb_t* trb);
xhci_trb_t* xhci_ring_dequeue(xhci_trb_t* ring, uint32_t* index, uint32_t size, uint32_t* cycle_state);

// Context management functions
xhci_device_context_t* xhci_allocate_device_context(xhci_controller_t* controller);
xhci_input_context_t* xhci_allocate_input_context(xhci_controller_t* controller);
void xhci_free_device_context(xhci_controller_t* controller, xhci_device_context_t* ctx);
void xhci_free_input_context(xhci_controller_t* controller, xhci_input_context_t* ctx);

// Register access functions
uint32_t xhci_read_cap_reg(xhci_controller_t* controller, uint32_t offset);
uint32_t xhci_read_op_reg(xhci_controller_t* controller, uint32_t offset);
void xhci_write_op_reg(xhci_controller_t* controller, uint32_t offset, uint32_t value);
uint32_t xhci_read_rt_reg(xhci_controller_t* controller, uint32_t offset);
void xhci_write_rt_reg(xhci_controller_t* controller, uint32_t offset, uint32_t value);
void xhci_write_doorbell(xhci_controller_t* controller, uint8_t doorbell, uint32_t value);

#endif
