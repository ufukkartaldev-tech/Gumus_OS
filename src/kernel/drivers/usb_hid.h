#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include "driver.h"
#include "hardware_detect.h"

// USB Sabitleri
#define USB_BASE_ADDRESS 0x0000FE00

// USB Command Registers
#define USBCMD_RUN_STOP      (1 << 0)
#define USBCMD_RESET         (1 << 1)
#define USBCMD_INT_THRESHOLD (1 << 3)
#define USBCMD_ASYNC_SCHED   (1 << 4)
#define USBCMD_PERIODIC_SCHED (1 << 5)
#define USBCMD_FRAME_LIST_SIZE (1 << 7)

// USB Status Registers
#define USBSTS_HALTED        (1 << 0)
#define USBSTS_PROCESS_ERROR (1 << 1)
#define USBSTS_SYSTEM_ERROR  (1 << 2)
#define USBSTS_INT_ASYNC     (1 << 3)
#define USBSTS_INT_PERIODIC  (1 << 4)
#define USBSTS_INT_HOST_ERR  (1 << 5)
#define USBSTS_INT_ASYNC_ADV (1 << 6)
#define USBSTS_INT_PERIODIC_ADV (1 << 7)

// USB Port Status and Control
#define PORT_CONNECT         (1 << 0)
#define PORT_CONNECT_CHANGE  (1 << 1)
#define PORT_ENABLE          (1 << 2)
#define PORT_ENABLE_CHANGE   (1 << 3)
#define PORT_RESET           (1 << 4)
#define PORT_POWER           (1 << 8)
#define PORT_LOW_SPEED       (1 << 9)

// USB Setup Packet
typedef struct {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// USB Standard Device Requests
#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_CLEAR_FEATURE     0x01
#define USB_REQ_SET_FEATURE       0x03
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_DESCRIPTOR    0x07
#define USB_REQ_GET_CONFIGURATION  0x08
#define USB_REQ_SET_CONFIGURATION  0x09
#define USB_REQ_GET_INTERFACE      0x0A
#define USB_REQ_SET_INTERFACE      0x0B
#define USB_REQ_SYNCH_FRAME        0x0C

// USB Descriptor Types
#define USB_DESC_DEVICE            0x01
#define USB_DESC_CONFIGURATION     0x02
#define USB_DESC_INTERFACE         0x04
#define USB_DESC_ENDPOINT          0x05
#define USB_DESC_HID               0x21
#define USB_DESC_REPORT            0x22

// USB HID Class Requests
#define HID_REQ_GET_REPORT         0x01
#define HID_REQ_GET_IDLE           0x02
#define HID_REQ_GET_PROTOCOL       0x03
#define HID_REQ_SET_REPORT         0x09
#define HID_REQ_SET_IDLE           0x0A
#define HID_REQ_SET_PROTOCOL       0x0B

// USB HID Usage Pages
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_KEYBOARD        0x07
#define HID_USAGE_PAGE_BUTTON          0x09

// USB HID Usage (Generic Desktop)
#define HID_USAGE_X             0x30
#define HID_USAGE_Y             0x31
#define HID_USAGE_WHEEL         0x38

// USB HID Usage (Keyboard)
#define HID_USAGE_LEFT_CTRL     0xE0
#define HID_USAGE_LEFT_SHIFT    0xE1
#define HID_USAGE_LEFT_ALT      0xE2
#define HID_USAGE_LEFT_GUI      0xE3
#define HID_USAGE_RIGHT_CTRL    0xE4
#define HID_USAGE_RIGHT_SHIFT   0xE5
#define HID_USAGE_RIGHT_ALT     0xE6
#define HID_USAGE_RIGHT_GUI     0xE7

// USB Device Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed)) usb_device_desc_t;

// USB Configuration Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __attribute__((packed)) usb_config_desc_t;

// USB Interface Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} __attribute__((packed)) usb_interface_desc_t;

// USB Endpoint Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed)) usb_endpoint_desc_t;

// USB HID Descriptor
typedef struct {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdHID;
    uint8_t  bCountryCode;
    uint8_t  bNumDescriptors;
    uint8_t  bDescriptorType1;
    uint16_t wDescriptorLength;
} __attribute__((packed)) usb_hid_desc_t;

// USB Host Controller Registers
typedef struct {
    uint32_t USBCMD;
    uint32_t USBSTS;
    uint32_t USBINTR;
    uint32_t FRNUM;
    uint32_t FRBASEADD;
    uint32_t SOFMOD;
    uint32_t PORTSC[8]; // 8 ports
} __attribute__((packed)) usb_hc_t;

// USB Device Structure
typedef struct {
    uint8_t address;
    uint8_t configuration;
    uint8_t interface;
    uint8_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t device_class;
    uint8_t subclass;
    uint8_t protocol;
} usb_device_t;

// USB HID Device Structure
typedef struct {
    driver_t base;
    usb_device_t usb_dev;
    usb_hc_t* hc;
    uint8_t endpoint_in;
    uint8_t endpoint_out;
    uint8_t interrupt_endpoint;
    uint8_t report_buffer[64];
    uint8_t last_report[64];
    int initialized;
} usb_hid_device_t;

// USB Keyboard Structure
typedef struct {
    usb_hid_device_t base;
    uint8_t key_states[6];
    uint8_t modifier_keys;
    uint8_t last_key_states[6];
    uint8_t last_modifier_keys;
    void (*key_handler)(uint8_t key, uint8_t pressed);
} usb_keyboard_t;

// USB Mouse Structure
typedef struct {
    usb_hid_device_t base;
    int8_t x, y;
    int8_t wheel;
    uint8_t buttons;
    uint8_t last_buttons;
    void (*move_handler)(int x, int y, int wheel);
    void (*button_handler)(uint8_t button, uint8_t pressed);
} usb_mouse_t;

// USB Fonksiyonları
int usb_init();
int usb_enumerate_device(usb_hc_t* hc, int port);
int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, void* buffer, uint16_t length);
int usb_set_address(usb_device_t* dev, uint8_t address);
int usb_set_configuration(usb_device_t* dev, uint8_t config);

// USB HID Fonksiyonları
int usb_hid_init(usb_hid_device_t* hid_dev);
int usb_hid_get_report(usb_hid_device_t* hid_dev, uint8_t report_id, void* buffer, uint16_t length);
int usb_hid_set_report(usb_hid_device_t* hid_dev, uint8_t report_id, void* buffer, uint16_t length);
int usb_hid_get_idle(usb_hid_device_t* hid_dev, uint8_t report_id, uint8_t* duration);
int usb_hid_set_idle(usb_hid_device_t* hid_dev, uint8_t report_id, uint8_t duration);

// USB Keyboard Fonksiyonları
int usb_keyboard_init(usb_keyboard_t* keyboard);
void usb_keyboard_process_report(usb_keyboard_t* keyboard, uint8_t* report);
uint8_t usb_key_to_scancode(uint8_t key);

// USB Mouse Fonksiyonları
int usb_mouse_init(usb_mouse_t* mouse);
void usb_mouse_process_report(usb_mouse_t* mouse, uint8_t* report);

// Sürücü Oluşturma Fonksiyonları
driver_t* create_usb_keyboard_driver(pci_device_t* device);
driver_t* create_usb_mouse_driver(pci_device_t* device);

#endif
