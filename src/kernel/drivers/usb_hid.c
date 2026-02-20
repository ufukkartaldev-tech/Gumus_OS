#include "usb_hid.h"
#include "../core/memory.h"
#include "../core/io.h"
#include "../core/string.h"
#include "../core/printf.h"

static usb_keyboard_t usb_keyboard;
static usb_mouse_t usb_mouse;
static int usb_keyboard_initialized = 0;
static int usb_mouse_initialized = 0;

// USB Keyboard Scancode Table
static uint8_t usb_to_scancode[256] = {
    0, 0, 0, 0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
    50, 49, 24, 25, 16, 19, 31, 10, 22, 47, 17, 45, 21, 44, 2, 3,
    4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
    68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
    84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
    116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
    132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147,
    148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163,
    164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195,
    196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
    212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
    228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243,
    244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

// USB Keyboard Driver Functions
static int usb_keyboard_driver_init(void) {
    printf("USB Keyboard sürücüsü başlatılıyor...\n");
    return usb_keyboard_init(&usb_keyboard);
}

static int usb_keyboard_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    // Keyboard input için buffer doldur
    if (size >= 8) {
        memcpy(buffer, usb_keyboard.report_buffer, 8);
        return 8;
    }
    return 0;
}

static int usb_keyboard_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    // Keyboard için write desteklenmiyor
    return -1;
}

static int usb_keyboard_driver_ioctl(uint32_t command, void* arg) {
    // IOCTL komutları için
    return 0;
}

static int usb_keyboard_driver_shutdown(void) {
    printf("USB Keyboard sürücüsü kapatılıyor...\n");
    usb_keyboard_initialized = 0;
    return 0;
}

// USB Mouse Driver Functions
static int usb_mouse_driver_init(void) {
    printf("USB Mouse sürücüsü başlatılıyor...\n");
    return usb_mouse_init(&usb_mouse);
}

static int usb_mouse_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    // Mouse input için buffer doldur
    if (size >= 4) {
        struct mouse_report {
            int8_t x, y, wheel;
            uint8_t buttons;
        } *report = (struct mouse_report*)buffer;
        
        report->x = usb_mouse.x;
        report->y = usb_mouse.y;
        report->wheel = usb_mouse.wheel;
        report->buttons = usb_mouse.buttons;
        
        return 4;
    }
    return 0;
}

static int usb_mouse_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    // Mouse için write desteklenmiyor
    return -1;
}

static int usb_mouse_driver_ioctl(uint32_t command, void* arg) {
    // IOCTL komutları için
    return 0;
}

static int usb_mouse_driver_shutdown(void) {
    printf("USB Mouse sürücüsü kapatılıyor...\n");
    usb_mouse_initialized = 0;
    return 0;
}

int usb_init() {
    printf("USB başlatılıyor...\n");
    return 0;
}

int usb_enumerate_device(usb_hc_t* hc, int port) {
    printf("USB aygıtı enumerate ediliyor: Port %d\n", port);
    
    // Port'u resetle
    hc->PORTSC[port] |= PORT_RESET;
    for (int i = 0; i < 1000; i++) {
        if (!(hc->PORTSC[port] & PORT_RESET)) {
            break;
        }
    }
    
    // Port'u enable et
    hc->PORTSC[port] |= PORT_ENABLE;
    
    return 0;
}

int usb_get_descriptor(usb_device_t* dev, uint8_t type, uint8_t index, void* buffer, uint16_t length) {
    // Setup packet oluştur
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x80; // Device to host
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (type << 8) | index;
    setup.wIndex = 0;
    setup.wLength = length;
    
    // Bu fonksiyon gerçek USB controller ile implement edilmeli
    printf("USB descriptor alınıyor: Type %d, Index %d\n", type, index);
    
    return 0;
}

int usb_set_address(usb_device_t* dev, uint8_t address) {
    printf("USB adres ayarlanıyor: %d\n", address);
    dev->address = address;
    return 0;
}

int usb_set_configuration(usb_device_t* dev, uint8_t config) {
    printf("USB konfigürasyon ayarlanıyor: %d\n", config);
    dev->configuration = config;
    return 0;
}

int usb_hid_init(usb_hid_device_t* hid_dev) {
    if (!hid_dev) return -1;
    
    printf("USB HID aygıtı başlatılıyor...\n");
    
    // HID descriptor'ı al
    usb_hid_desc_t hid_desc;
    if (usb_get_descriptor(&hid_dev->usb_dev, USB_DESC_HID, 0, &hid_desc, sizeof(hid_desc)) != 0) {
        printf("HID descriptor alınamadı\n");
        return -1;
    }
    
    // Report descriptor'ı al
    uint8_t report_desc[256];
    if (usb_get_descriptor(&hid_dev->usb_dev, USB_DESC_REPORT, 0, report_desc, hid_desc.wDescriptorLength) != 0) {
        printf("Report descriptor alınamadı\n");
        return -1;
    }
    
    hid_dev->initialized = 1;
    return 0;
}

int usb_hid_get_report(usb_hid_device_t* hid_dev, uint8_t report_id, void* buffer, uint16_t length) {
    if (!hid_dev || !hid_dev->initialized) return -1;
    
    // HID GET_REPORT komutu gönder
    printf("HID report alınıyor: ID %d\n", report_id);
    
    return 0;
}

int usb_hid_set_report(usb_hid_device_t* hid_dev, uint8_t report_id, void* buffer, uint16_t length) {
    if (!hid_dev || !hid_dev->initialized) return -1;
    
    // HID SET_REPORT komutu gönder
    printf("HID report ayarlanıyor: ID %d\n", report_id);
    
    return 0;
}

int usb_hid_get_idle(usb_hid_device_t* hid_dev, uint8_t report_id, uint8_t* duration) {
    if (!hid_dev || !hid_dev->initialized) return -1;
    
    printf("HID idle süresi alınıyor: ID %d\n", report_id);
    
    return 0;
}

int usb_hid_set_idle(usb_hid_device_t* hid_dev, uint8_t report_id, uint8_t duration) {
    if (!hid_dev || !hid_dev->initialized) return -1;
    
    printf("HID idle süresi ayarlanıyor: ID %d, Süre %d\n", report_id, duration);
    
    return 0;
}

int usb_keyboard_init(usb_keyboard_t* keyboard) {
    if (!keyboard) return -1;
    
    printf("USB Keyboard başlatılıyor...\n");
    
    // HID olarak başlat
    if (usb_hid_init(&keyboard->base) != 0) {
        return -1;
    }
    
    // Keyboard state'ini sıfırla
    memset(keyboard->key_states, 0, 6);
    memset(keyboard->last_key_states, 0, 6);
    keyboard->modifier_keys = 0;
    keyboard->last_modifier_keys = 0;
    
    usb_keyboard_initialized = 1;
    return 0;
}

void usb_keyboard_process_report(usb_keyboard_t* keyboard, uint8_t* report) {
    if (!keyboard || !report) return;
    
    // Modifier keys kontrolü
    uint8_t modifiers = report[0];
    
    // Tuş basma/bırakma olaylarını kontrol et
    for (int i = 0; i < 6; i++) {
        uint8_t key = report[i+2];
        
        // Tuşun daha önce basılıp basılmadığını kontrol et
        int key_pressed = 0;
        for (int j = 0; j < 6; j++) {
            if (keyboard->last_key_states[j] == key) {
                key_pressed = 1;
                break;
            }
        }
        
        if (!key_pressed && key != 0) {
            // Yeni tuş basıldı
            uint8_t scancode = usb_key_to_scancode(key);
            if (keyboard->key_handler) {
                keyboard->key_handler(scancode, 1); // Pressed
            }
        }
    }
    
    // Bırakılan tuşları kontrol et
    for (int i = 0; i < 6; i++) {
        uint8_t key = keyboard->last_key_states[i];
        
        if (key != 0) {
            int still_pressed = 0;
            for (int j = 0; j < 6; j++) {
                if (report[j+2] == key) {
                    still_pressed = 1;
                    break;
                }
            }
            
            if (!still_pressed) {
                // Tuş bırakıldı
                uint8_t scancode = usb_key_to_scancode(key);
                if (keyboard->key_handler) {
                    keyboard->key_handler(scancode, 0); // Released
                }
            }
        }
    }
    
    // State'i güncelle
    memcpy(keyboard->last_key_states, &report[2], 6);
    keyboard->last_modifier_keys = modifiers;
}

uint8_t usb_key_to_scancode(uint8_t key) {
    if (key < 256) {
        return usb_to_scancode[key];
    }
    return 0;
}

int usb_mouse_init(usb_mouse_t* mouse) {
    if (!mouse) return -1;
    
    printf("USB Mouse başlatılıyor...\n");
    
    // HID olarak başlat
    if (usb_hid_init(&mouse->base) != 0) {
        return -1;
    }
    
    // Mouse state'ini sıfırla
    mouse->x = 0;
    mouse->y = 0;
    mouse->wheel = 0;
    mouse->buttons = 0;
    mouse->last_buttons = 0;
    
    usb_mouse_initialized = 1;
    return 0;
}

void usb_mouse_process_report(usb_mouse_t* mouse, uint8_t* report) {
    if (!mouse || !report) return;
    
    // Mouse report formatı: buttons, x, y, wheel
    uint8_t buttons = report[0];
    int8_t x = report[1];
    int8_t y = report[2];
    int8_t wheel = report[3];
    
    // Hareket olayı
    if (x != 0 || y != 0 || wheel != 0) {
        if (mouse->move_handler) {
            mouse->move_handler(x, y, wheel);
        }
    }
    
    // Buton değişikliklerini kontrol et
    if (buttons != mouse->last_buttons) {
        for (int i = 0; i < 8; i++) {
            uint8_t button_mask = 1 << i;
            if ((buttons & button_mask) != (mouse->last_buttons & button_mask)) {
                if (mouse->button_handler) {
                    mouse->button_handler(i, (buttons & button_mask) ? 1 : 0);
                }
            }
        }
    }
    
    // State'i güncelle
    mouse->x = x;
    mouse->y = y;
    mouse->wheel = wheel;
    mouse->buttons = buttons;
    mouse->last_buttons = buttons;
}

driver_t* create_usb_keyboard_driver(pci_device_t* device) {
    if (usb_keyboard_initialized) {
        printf("USB Keyboard zaten başlatılmış\n");
        return &usb_keyboard.base.base;
    }
    
    // Sürücü yapısını ayarla
    strcpy(usb_keyboard.base.base.name, "USB Keyboard");
    usb_keyboard.base.base.type = DRIVER_TYPE_INPUT;
    usb_keyboard.base.base.init = usb_keyboard_driver_init;
    usb_keyboard.base.base.read = usb_keyboard_driver_read;
    usb_keyboard.base.base.write = usb_keyboard_driver_write;
    usb_keyboard.base.base.ioctl = usb_keyboard_driver_ioctl;
    usb_keyboard.base.base.shutdown = usb_keyboard_driver_shutdown;
    
    printf("USB Keyboard sürücüsü oluşturuldu\n");
    return &usb_keyboard.base.base;
}

driver_t* create_usb_mouse_driver(pci_device_t* device) {
    if (usb_mouse_initialized) {
        printf("USB Mouse zaten başlatılmış\n");
        return &usb_mouse.base.base;
    }
    
    // Sürücü yapısını ayarla
    strcpy(usb_mouse.base.base.name, "USB Mouse");
    usb_mouse.base.base.type = DRIVER_TYPE_INPUT;
    usb_mouse.base.base.init = usb_mouse_driver_init;
    usb_mouse.base.base.read = usb_mouse_driver_read;
    usb_mouse.base.base.write = usb_mouse_driver_write;
    usb_mouse.base.base.ioctl = usb_mouse_driver_ioctl;
    usb_mouse.base.base.shutdown = usb_mouse_driver_shutdown;
    
    printf("USB Mouse sürücüsü oluşturuldu\n");
    return &usb_mouse.base.base;
}
