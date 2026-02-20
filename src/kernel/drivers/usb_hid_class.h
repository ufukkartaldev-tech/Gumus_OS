#ifndef USB_HID_CLASS_H
#define USB_HID_CLASS_H

#include "usb_host.h"
#include "../core/types.h"

// USB HID Class Constants
#define USB_HID_CLASS              0x03
#define USB_HID_SUBCLASS_NONE      0x00
#define USB_HID_SUBCLASS_BOOT      0x01
#define USB_HID_PROTOCOL_NONE      0x00
#define USB_HID_PROTOCOL_KEYBOARD  0x01
#define USB_HID_PROTOCOL_MOUSE     0x02

// USB HID Class Requests
#define HID_REQ_GET_REPORT         0x01
#define HID_REQ_GET_IDLE           0x02
#define HID_REQ_GET_PROTOCOL       0x03
#define HID_REQ_SET_REPORT         0x09
#define HID_REQ_SET_IDLE           0x0A
#define HID_REQ_SET_PROTOCOL       0x0B

// USB HID Descriptor Types
#define HID_DESC_TYPE_HID          0x21
#define HID_DESC_TYPE_REPORT       0x22
#define HID_DESC_TYPE_PHYSICAL     0x23

// USB HID Report Types
#define HID_REPORT_TYPE_INPUT      0x01
#define HID_REPORT_TYPE_OUTPUT     0x02
#define HID_REPORT_TYPE_FEATURE    0x03

// USB HID Usage Pages
#define HID_USAGE_PAGE_GENERIC      0x01
#define HID_USAGE_PAGE_SIMULATION   0x02
#define HID_USAGE_PAGE_VR           0x03
#define HID_USAGE_PAGE_SPORT        0x04
#define HID_USAGE_PAGE_GAME         0x05
#define HID_USAGE_PAGE_KEYBOARD     0x07
#define HID_USAGE_PAGE_LED          0x08
#define HID_USAGE_PAGE_BUTTON       0x09
#define HID_USAGE_PAGE_ORDINAL      0x0A
#define HID_USAGE_PAGE_TELEPHONY    0x0B
#define HID_USAGE_PAGE_CONSUMER     0x0C
#define HID_USAGE_PAGE_DIGITIZER    0x0D
#define HID_USAGE_PAGE_UNICODE      0x10
#define HID_USAGE_PAGE_ALPHANUMERIC 0x14

// USB HID Usage (Generic Desktop)
#define HID_USAGE_POINTER           0x01
#define HID_USAGE_MOUSE             0x02
#define HID_USAGE_JOYSTICK          0x04
#define HID_USAGE_GAME_PAD          0x05
#define HID_USAGE_KEYBOARD          0x06
#define HID_USAGE_X                 0x30
#define HID_USAGE_Y                 0x31
#define HID_USAGE_Z                 0x32
#define HID_USAGE_RX                0x33
#define HID_USAGE_RY                0x34
#define HID_USAGE_RZ                0x35
#define HID_USAGE_SLIDER            0x36
#define HID_USAGE_DIAL              0x37
#define HID_USAGE_WHEEL             0x38
#define HID_USAGE_HAT_SWITCH        0x39

// USB HID Usage (Keyboard)
#define HID_USAGE_KEYBOARD_NONE           0x00
#define HID_USAGE_KEYBOARD_ERROR_ROLLOVER  0x01
#define HID_USAGE_KEYBOARD_POST_FAIL       0x02
#define HID_USAGE_KEYBOARD_ERROR_UNDEFINED 0x03
#define HID_USAGE_KEYBOARD_A               0x04
#define HID_USAGE_KEYBOARD_B               0x05
#define HID_USAGE_KEYBOARD_C               0x06
#define HID_USAGE_KEYBOARD_D               0x07
#define HID_USAGE_KEYBOARD_E               0x08
#define HID_USAGE_KEYBOARD_F               0x09
#define HID_USAGE_KEYBOARD_G               0x0A
#define HID_USAGE_KEYBOARD_H               0x0B
#define HID_USAGE_KEYBOARD_I               0x0C
#define HID_USAGE_KEYBOARD_J               0x0D
#define HID_USAGE_KEYBOARD_K               0x0E
#define HID_USAGE_KEYBOARD_L               0x0F
#define HID_USAGE_KEYBOARD_M               0x10
#define HID_USAGE_KEYBOARD_N               0x11
#define HID_USAGE_KEYBOARD_O               0x12
#define HID_USAGE_KEYBOARD_P               0x13
#define HID_USAGE_KEYBOARD_Q               0x14
#define HID_USAGE_KEYBOARD_R               0x15
#define HID_USAGE_KEYBOARD_S               0x16
#define HID_USAGE_KEYBOARD_T               0x17
#define HID_USAGE_KEYBOARD_U               0x18
#define HID_USAGE_KEYBOARD_V               0x19
#define HID_USAGE_KEYBOARD_W               0x1A
#define HID_USAGE_KEYBOARD_X               0x1B
#define HID_USAGE_KEYBOARD_Y               0x1C
#define HID_USAGE_KEYBOARD_Z               0x1D
#define HID_USAGE_KEYBOARD_1               0x1E
#define HID_USAGE_KEYBOARD_2               0x1F
#define HID_USAGE_KEYBOARD_3               0x20
#define HID_USAGE_KEYBOARD_4               0x21
#define HID_USAGE_KEYBOARD_5               0x22
#define HID_USAGE_KEYBOARD_6               0x23
#define HID_USAGE_KEYBOARD_7               0x24
#define HID_USAGE_KEYBOARD_8               0x25
#define HID_USAGE_KEYBOARD_9               0x26
#define HID_USAGE_KEYBOARD_0               0x27
#define HID_USAGE_KEYBOARD_ENTER           0x28
#define HID_USAGE_KEYBOARD_ESCAPE          0x29
#define HID_USAGE_KEYBOARD_BACKSPACE       0x2A
#define HID_USAGE_KEYBOARD_TAB             0x2B
#define HID_USAGE_KEYBOARD_SPACE           0x2C
#define HID_USAGE_KEYBOARD_CAPS_LOCK       0x39
#define HID_USAGE_KEYBOARD_F1              0x3A
#define HID_USAGE_KEYBOARD_F2              0x3B
#define HID_USAGE_KEYBOARD_F3              0x3C
#define HID_USAGE_KEYBOARD_F4              0x3D
#define HID_USAGE_KEYBOARD_F5              0x3E
#define HID_USAGE_KEYBOARD_F6              0x3F
#define HID_USAGE_KEYBOARD_F7              0x40
#define HID_USAGE_KEYBOARD_F8              0x41
#define HID_USAGE_KEYBOARD_F9              0x42
#define HID_USAGE_KEYBOARD_F10             0x43
#define HID_USAGE_KEYBOARD_F11             0x44
#define HID_USAGE_KEYBOARD_F12             0x45

// USB HID Usage (Keyboard Modifiers)
#define HID_USAGE_KEYBOARD_LEFT_CTRL       0xE0
#define HID_USAGE_KEYBOARD_LEFT_SHIFT      0xE1
#define HID_USAGE_KEYBOARD_LEFT_ALT        0xE2
#define HID_USAGE_KEYBOARD_LEFT_GUI        0xE3
#define HID_USAGE_KEYBOARD_RIGHT_CTRL      0xE4
#define HID_USAGE_KEYBOARD_RIGHT_SHIFT     0xE5
#define HID_USAGE_KEYBOARD_RIGHT_ALT       0xE6
#define HID_USAGE_KEYBOARD_RIGHT_GUI       0xE7

// USB HID Usage (Button)
#define HID_USAGE_BUTTON_1          0x01
#define HID_USAGE_BUTTON_2          0x02
#define HID_USAGE_BUTTON_3          0x03
#define HID_USAGE_BUTTON_4          0x04
#define HID_USAGE_BUTTON_5          0x05
#define HID_USAGE_BUTTON_6          0x06
#define HID_USAGE_BUTTON_7          0x07
#define HID_USAGE_BUTTON_8          0x08

// USB HID Report Item Types
#define HID_ITEM_TYPE_MAIN          0x00
#define HID_ITEM_TYPE_GLOBAL        0x04
#define HID_ITEM_TYPE_LOCAL         0x08

// USB HID Main Item Tags
#define HID_MAIN_ITEM_TAG_INPUT     0x08
#define HID_MAIN_ITEM_TAG_OUTPUT    0x09
#define HID_MAIN_ITEM_TAG_FEATURE   0x0B
#define HID_MAIN_ITEM_TAG_COLLECTION 0x0A
#define HID_MAIN_ITEM_TAG_END_COLLECTION 0x0C

// USB HID Global Item Tags
#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE    0x00
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MIN   0x01
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAX   0x02
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MIN  0x03
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAX  0x04
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT 0x05
#define HID_GLOBAL_ITEM_TAG_UNIT          0x06
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE   0x07
#define HID_GLOBAL_ITEM_TAG_REPORT_ID     0x08
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT  0x09
#define HID_GLOBAL_ITEM_TAG_PUSH          0x0A
#define HID_GLOBAL_ITEM_TAG_POP           0x0B

// USB HID Local Item Tags
#define HID_LOCAL_ITEM_TAG_USAGE         0x00
#define HID_LOCAL_ITEM_TAG_USAGE_MIN     0x01
#define HID_LOCAL_ITEM_TAG_USAGE_MAX     0x02
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX 0x03
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MIN   0x04
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAX   0x05
#define HID_LOCAL_ITEM_TAG_STRING_INDEX   0x07
#define HID_LOCAL_ITEM_TAG_STRING_MIN     0x08
#define HID_LOCAL_ITEM_TAG_STRING_MAX     0x09
#define HID_LOCAL_ITEM_TAG_DELIMITER      0x0A

// USB HID Report Descriptor Item
typedef struct {
    uint8_t size : 2;
    uint8_t type : 2;
    uint8_t tag : 4;
} __attribute__((packed)) hid_item_prefix_t;

// USB HID Descriptor
typedef struct {
    uint8_t length;
    uint8_t type;
    uint16_t bcd_hid;
    uint8_t country_code;
    uint8_t num_descriptors;
    uint8_t descriptor_type;
    uint16_t descriptor_length;
} __attribute__((packed)) usb_hid_descriptor_t;

// USB HID Device Structure
typedef struct usb_hid_device {
    usb_device_t* usb_device;
    
    // Endpoints
    uint8_t interrupt_in_endpoint;
    uint16_t max_packet_size;
    uint8_t poll_interval;
    
    // HID information
    usb_hid_descriptor_t hid_descriptor;
    uint8_t* report_descriptor;
    uint16_t report_descriptor_length;
    
    // Report buffers
    uint8_t input_report_buffer[64];
    uint8_t output_report_buffer[64];
    uint8_t feature_report_buffer[64];
    
    // Device type
    uint8_t hid_type; // 0=Unknown, 1=Keyboard, 2=Mouse, 3=Other
    uint8_t protocol;
    
    // State
    uint8_t initialized;
    uint8_t idle_rate;
    
    struct usb_hid_device* next;
} usb_hid_device_t;

// USB Keyboard Structure
typedef struct {
    usb_hid_device_t base;
    
    // Keyboard state
    uint8_t modifier_keys;
    uint8_t key_states[6];
    uint8_t last_modifier_keys;
    uint8_t last_key_states[6];
    
    // Event handlers
    void (*key_handler)(uint8_t key, uint8_t pressed);
    void (*modifier_handler)(uint8_t modifier, uint8_t pressed);
} usb_keyboard_t;

// USB Mouse Structure
typedef struct {
    usb_hid_device_t base;
    
    // Mouse state
    int32_t x, y;
    int32_t wheel;
    uint8_t buttons;
    uint8_t last_buttons;
    
    // Event handlers
    void (*move_handler)(int32_t x, int32_t y, int32_t wheel);
    void (*button_handler)(uint8_t button, uint8_t pressed);
} usb_mouse_t;

// USB HID Driver Structure
typedef struct {
    driver_t base;
    usb_hid_device_t* devices;
    uint32_t device_count;
} usb_hid_driver_t;

// Function Prototypes
int usb_hid_init();
int usb_hid_probe(usb_device_t* usb_device);
int usb_hid_remove(usb_device_t* usb_device);
int usb_hid_get_report(usb_hid_device_t* device, uint8_t report_id, 
                      uint8_t report_type, uint8_t* buffer, uint16_t length);
int usb_hid_set_report(usb_hid_device_t* device, uint8_t report_id, 
                      uint8_t report_type, const uint8_t* buffer, uint16_t length);
int usb_hid_get_idle(usb_hid_device_t* device, uint8_t report_id, uint8_t* duration);
int usb_hid_set_idle(usb_hid_device_t* device, uint8_t report_id, uint8_t duration);
int usb_hid_get_protocol(usb_hid_device_t* device, uint8_t* protocol);
int usb_hid_set_protocol(usb_hid_device_t* device, uint8_t protocol);
int usb_hid_parse_report_descriptor(usb_hid_device_t* device);

// Keyboard functions
int usb_keyboard_init(usb_keyboard_t* keyboard);
void usb_keyboard_process_report(usb_keyboard_t* keyboard, const uint8_t* report);
uint8_t usb_key_to_scancode(uint8_t key);
const char* usb_key_to_string(uint8_t key);

// Mouse functions
int usb_mouse_init(usb_mouse_t* mouse);
void usb_mouse_process_report(usb_mouse_t* mouse, const uint8_t* report);

// Device management
usb_hid_device_t* usb_hid_find_device(usb_device_t* usb_device);
int usb_hid_add_device(usb_hid_device_t* device);
int usb_hid_remove_device(usb_hid_device_t* device);
void usb_hid_list_devices();

// Driver interface functions
driver_t* create_usb_hid_driver();

#endif
