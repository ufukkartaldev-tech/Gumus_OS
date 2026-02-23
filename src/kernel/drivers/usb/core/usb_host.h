#ifndef USB_HOST_H
#define USB_HOST_H

#include "types.h"
#include "driver.h"
#include "hardware_detect.h"

// USB StandartlarÄ±
#define USB_SPEC_1_1    0x0110
#define USB_SPEC_2_0    0x0200
#define USB_SPEC_3_0    0x0300

// USB Speed'ler
typedef enum {
    USB_SPEED_LOW = 0,
    USB_SPEED_FULL = 1,
    USB_SPEED_HIGH = 2,
    USB_SPEED_SUPER = 3
} usb_speed_t;

// USB Transfer Tipleri
typedef enum {
    USB_TRANSFER_CONTROL = 0,
    USB_TRANSFER_ISOCHRONOUS = 1,
    USB_TRANSFER_BULK = 2,
    USB_TRANSFER_INTERRUPT = 3
} usb_transfer_type_t;

// USB Direction
typedef enum {
    USB_DIRECTION_OUT = 0,
    USB_DIRECTION_IN = 1
} usb_direction_t;

// USB AygÄ±t Tipi
typedef enum {
    USB_DEVICE_TYPE_UNKNOWN = 0x00,
    USB_DEVICE_TYPE_AUDIO = 0x01,
    USB_DEVICE_TYPE_COMMUNICATIONS = 0x02,
    USB_DEVICE_TYPE_HID = 0x03,
    USB_DEVICE_TYPE_PHYSICAL = 0x05,
    USB_DEVICE_TYPE_IMAGING = 0x06,
    USB_DEVICE_TYPE_PRINTER = 0x07,
    USB_DEVICE_TYPE_STORAGE = 0x08,
    USB_DEVICE_TYPE_HUB = 0x09,
    USB_DEVICE_TYPE_CDC_DATA = 0x0A,
    USB_DEVICE_TYPE_SMART_CARD = 0x0B,
    USB_DEVICE_TYPE_CONTENT_SECURITY = 0x0D,
    USB_DEVICE_TYPE_VIDEO = 0x0E,
    USB_DEVICE_TYPE_PERSONAL_HEALTHCARE = 0x0F,
    USB_DEVICE_TYPE_AUDIO_VIDEO = 0x10,
    USB_DEVICE_TYPE_BILLBOARD = 0x11,
    USB_DEVICE_TYPE_TYPE_C_BRIDGE = 0x0D,
    USB_DEVICE_TYPE_DIAGNOSTIC = 0xDC,
    USB_DEVICE_TYPE_WIRELESS = 0xE0,
    USB_DEVICE_TYPE_MISCELLANEOUS = 0xEF,
    USB_DEVICE_TYPE_VENDOR_SPECIFIC = 0xFF
} usb_device_type_t;

// USB Endpoint Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t address;
    uint8_t attributes;
    uint16_t max_packet_size;
    uint8_t interval;
} usb_endpoint_descriptor_t;

// USB Interface Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint8_t number;
    uint8_t alternate;
    uint8_t num_endpoints;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol;
    uint8_t interface;
} usb_interface_descriptor_t;

// USB Configuration Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint16_t total_length;
    uint8_t num_interfaces;
    uint8_t config_value;
    uint8_t config_string;
    uint8_t attributes;
    uint8_t max_power;
} usb_config_descriptor_t;

// USB Device Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint16_t usb_version;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol;
    uint8_t max_packet_size0;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_version;
    uint8_t manufacturer_string;
    uint8_t product_string;
    uint8_t serial_string;
    uint8_t num_configurations;
} usb_device_descriptor_t;

// USB AygÄ±t YapÄ±sÄ±
typedef struct usb_device {
    uint32_t id;
    uint8_t address;
    uint8_t port;
    usb_speed_t speed;
    usb_device_type_t type;
    
    // Descriptor'lar
    usb_device_descriptor_t device_desc;
    usb_config_descriptor_t config_desc;
    usb_interface_descriptor_t interface_desc;
    usb_endpoint_descriptor_t endpoint_desc;
    
    // Host controller baÄŸlantÄ±sÄ±
    void* host_controller;
    struct usb_device* next;
} usb_device_t;

// USB Host Controller Interface
typedef struct usb_host_controller {
    driver_t base;
    uint32_t type; // OHCI, EHCI, XHCI
    
    // Controller fonksiyonlarÄ±
    int (*init)(struct usb_host_controller* controller);
    int (*reset)(struct usb_host_controller* controller);
    int (*enumerate_device)(struct usb_host_controller* controller, uint8_t port);
    int (*control_transfer)(struct usb_host_controller* controller, 
                         uint8_t device_addr, uint8_t endpoint,
                         uint8_t* setup_packet, uint8_t* data, uint32_t length);
    int (*bulk_transfer)(struct usb_host_controller* controller,
                       uint8_t device_addr, uint8_t endpoint,
                       uint8_t* data, uint32_t length, uint8_t direction);
    int (*interrupt_transfer)(struct usb_host_controller* controller,
                          uint8_t device_addr, uint8_t endpoint,
                          uint8_t* data, uint32_t length);
    
    // Controller Ã¶zellikleri
    uint32_t num_ports;
    uint32_t supported_speeds;
    void* mmio_base;
    uint32_t irq_line;
    
    struct usb_host_controller* next;
} usb_host_controller_t;

// USB Host Controller Tipleri
#define USB_HC_OHCI  0x01
#define USB_HC_EHCI  0x02
#define USB_HC_XHCI  0x03

// USB Setup Packet
typedef struct {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} usb_setup_packet_t;

// USB Standart Ä°stekleri
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE      0x03
#define USB_REQ_SET_ADDRESS      0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION 0x08
#define USB_REQ_SET_CONFIGURATION 0x09
#define USB_REQ_GET_INTERFACE    0x0A
#define USB_REQ_SET_INTERFACE    0x0B
#define USB_REQ_SYNCH_FRAME     0x0C

// USB Descriptor Tipleri
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONFIG         0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT       0x05

// USB Fonksiyon Prototipleri
void usb_host_init();
int usb_host_register_controller(usb_host_controller_t* controller);
int usb_host_unregister_controller(usb_host_controller_t* controller);
int usb_host_enumerate_device(usb_host_controller_t* controller, uint8_t port);
int usb_host_control_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                           uint8_t endpoint, usb_setup_packet_t* setup,
                           uint8_t* data, uint32_t length);
int usb_host_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                        uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction);
int usb_host_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr,
                            uint8_t endpoint, uint8_t* data, uint32_t length);
void usb_host_list_devices();
void usb_host_list_controllers();

// SÃ¼rÃ¼cÃ¼ oluÅŸturma fonksiyonlarÄ±
driver_t* create_ohci_driver(pci_device_t* device);
driver_t* create_ehci_driver(pci_device_t* device);
driver_t* create_xhci_driver(pci_device_t* device);

#endif
