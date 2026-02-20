#include "usb_hid_class.h"
#include "../core/memory.h"
#include "string.h"
#include "stdio.h"

// Global HID driver
static usb_hid_driver_t hid_driver;

// Global device list
static usb_hid_device_t* hid_devices = NULL;

// Key mapping table
static const char* key_names[] = {
    "None", "ErrorRollover", "PostFail", "ErrorUndefined",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    "Enter", "Escape", "Backspace", "Tab", "Space", "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    "PrintScreen", "ScrollLock", "Pause", "Insert", "Home", "PageUp",
    "Delete", "End", "PageDown", "RightArrow", "LeftArrow", "DownArrow", "UpArrow"
};

// Initialize USB HID driver
int usb_hid_init() {
    printf("USB HID: Sürücü başlatılıyor\n");
    
    hid_driver.devices = NULL;
    hid_driver.device_count = 0;
    
    strcpy(hid_driver.base.name, "USB HID Driver");
    hid_driver.base.type = DRIVER_TYPE_INPUT;
    hid_driver.base.class = DRIVER_CLASS_SERIAL;
    hid_driver.base.vendor_id = 0;
    hid_driver.base.device_id = 0;
    
    printf("USB HID: Sürücü başlatıldı\n");
    return 0;
}

// Get HID report
int usb_hid_get_report(usb_hid_device_t* device, uint8_t report_id, 
                      uint8_t report_type, uint8_t* buffer, uint16_t length) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0xA1; // Device-to-host, Class, Interface
    setup.request = HID_REQ_GET_REPORT;
    setup.value = (report_type << 8) | report_id;
    setup.index = 0; // Interface number
    setup.length = length;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        buffer,
        length
    );
    
    if (result != 0) {
        printf("USB HID: Report alma başarısız\n");
        return -1;
    }
    
    return 0;
}

// Set HID report
int usb_hid_set_report(usb_hid_device_t* device, uint8_t report_id, 
                      uint8_t report_type, const uint8_t* buffer, uint16_t length) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = HID_REQ_SET_REPORT;
    setup.value = (report_type << 8) | report_id;
    setup.index = 0; // Interface number
    setup.length = length;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        (uint8_t*)buffer,
        length
    );
    
    if (result != 0) {
        printf("USB HID: Report ayarlama başarısız\n");
        return -1;
    }
    
    return 0;
}

// Get idle rate
int usb_hid_get_idle(usb_hid_device_t* device, uint8_t report_id, uint8_t* duration) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0xA1; // Device-to-host, Class, Interface
    setup.request = HID_REQ_GET_IDLE;
    setup.value = report_id;
    setup.index = 0; // Interface number
    setup.length = 1;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        duration,
        1
    );
    
    if (result != 0) {
        printf("USB HID: Idle rate alma başarısız\n");
        return -1;
    }
    
    return 0;
}

// Set idle rate
int usb_hid_set_idle(usb_hid_device_t* device, uint8_t report_id, uint8_t duration) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = HID_REQ_SET_IDLE;
    setup.value = (duration << 8) | report_id;
    setup.index = 0; // Interface number
    setup.length = 0;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        NULL,
        0
    );
    
    if (result != 0) {
        printf("USB HID: Idle rate ayarlama başarısız\n");
        return -1;
    }
    
    device->idle_rate = duration;
    return 0;
}

// Get protocol
int usb_hid_get_protocol(usb_hid_device_t* device, uint8_t* protocol) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0xA1; // Device-to-host, Class, Interface
    setup.request = HID_REQ_GET_PROTOCOL;
    setup.value = 0;
    setup.index = 0; // Interface number
    setup.length = 1;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        protocol,
        1
    );
    
    if (result != 0) {
        printf("USB HID: Protocol alma başarısız\n");
        return -1;
    }
    
    return 0;
}

// Set protocol
int usb_hid_set_protocol(usb_hid_device_t* device, uint8_t protocol) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = HID_REQ_SET_PROTOCOL;
    setup.value = protocol;
    setup.index = 0; // Interface number
    setup.length = 0;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        NULL,
        0
    );
    
    if (result != 0) {
        printf("USB HID: Protocol ayarlama başarısız\n");
        return -1;
    }
    
    device->protocol = protocol;
    return 0;
}

// Parse report descriptor (simplified)
int usb_hid_parse_report_descriptor(usb_hid_device_t* device) {
    if (!device->report_descriptor || device->report_descriptor_length == 0) {
        printf("USB HID: Report descriptor yok\n");
        return -1;
    }
    
    // This is a simplified parser
    // In a real implementation, this would parse the HID report descriptor
    // to understand the device's capabilities
    
    printf("USB HID: Report descriptor boyutu: %d bayt\n", device->report_descriptor_length);
    
    // Try to determine device type from report descriptor
    // This is a very basic heuristic
    bool has_keyboard = false;
    bool has_mouse = false;
    
    for (uint16_t i = 0; i < device->report_descriptor_length; i++) {
        if (device->report_descriptor[i] == 0x06) { // Usage Page
            i++;
            if (i < device->report_descriptor_length) {
                uint16_t page = device->report_descriptor[i];
                if (page == 0x07) { // Keyboard
                    has_keyboard = true;
                } else if (page == 0x01) { // Generic Desktop
                    has_mouse = true;
                }
            }
        }
    }
    
    if (has_keyboard) {
        device->hid_type = 1; // Keyboard
        printf("USB HID: Klavye tespit edildi\n");
    } else if (has_mouse) {
        device->hid_type = 2; // Mouse
        printf("USB HID: Fare tespit edildi\n");
    } else {
        device->hid_type = 3; // Other
        printf("USB HID: Diğer HID aygıtı\n");
    }
    
    return 0;
}

// Initialize keyboard
int usb_keyboard_init(usb_keyboard_t* keyboard) {
    if (!keyboard) {
        return -1;
    }
    
    memset(keyboard->key_states, 0, sizeof(keyboard->key_states));
    memset(keyboard->last_key_states, 0, sizeof(keyboard->last_key_states));
    keyboard->modifier_keys = 0;
    keyboard->last_modifier_keys = 0;
    
    printf("USB Keyboard: Başlatıldı\n");
    return 0;
}

// Process keyboard report
void usb_keyboard_process_report(usb_keyboard_t* keyboard, const uint8_t* report) {
    if (!keyboard || !report) {
        return;
    }
    
    // Extract modifier keys
    uint8_t new_modifier_keys = report[0];
    
    // Extract key states
    uint8_t new_key_states[6];
    memcpy(new_key_states, &report[2], 6);
    
    // Handle modifier key changes
    uint8_t changed_modifiers = new_modifier_keys ^ keyboard->last_modifier_keys;
    if (changed_modifiers) {
        for (uint8_t i = 0; i < 8; i++) {
            if (changed_modifiers & (1 << i)) {
                uint8_t modifier = 0xE0 + i; // Left Ctrl = 0xE0, etc.
                uint8_t pressed = (new_modifier_keys & (1 << i)) ? 1 : 0;
                
                if (keyboard->modifier_handler) {
                    keyboard->modifier_handler(modifier, pressed);
                }
                
                printf("USB Keyboard: Modifier %d %s\n", modifier, pressed ? "basıldı" : "bırakıldı");
            }
        }
    }
    
    // Handle key changes
    // Check for key releases
    for (uint8_t i = 0; i < 6; i++) {
        if (keyboard->last_key_states[i] != 0) {
            bool still_pressed = false;
            for (uint8_t j = 0; j < 6; j++) {
                if (new_key_states[j] == keyboard->last_key_states[i]) {
                    still_pressed = true;
                    break;
                }
            }
            
            if (!still_pressed) {
                if (keyboard->key_handler) {
                    keyboard->key_handler(keyboard->last_key_states[i], 0);
                }
                
                const char* key_name = (keyboard->last_key_states[i] < sizeof(key_names)/sizeof(key_names[0])) ?
                                      key_names[keyboard->last_key_states[i]] : "Unknown";
                printf("USB Keyboard: %s bırakıldı\n", key_name);
            }
        }
    }
    
    // Check for key presses
    for (uint8_t i = 0; i < 6; i++) {
        if (new_key_states[i] != 0) {
            bool was_pressed = false;
            for (uint8_t j = 0; j < 6; j++) {
                if (keyboard->last_key_states[j] == new_key_states[i]) {
                    was_pressed = true;
                    break;
                }
            }
            
            if (!was_pressed) {
                if (keyboard->key_handler) {
                    keyboard->key_handler(new_key_states[i], 1);
                }
                
                const char* key_name = (new_key_states[i] < sizeof(key_names)/sizeof(key_names[0])) ?
                                      key_names[new_key_states[i]] : "Unknown";
                printf("USB Keyboard: %s basıldı\n", key_name);
            }
        }
    }
    
    // Update state
    keyboard->last_modifier_keys = new_modifier_keys;
    memcpy(keyboard->last_key_states, new_key_states, 6);
}

// Initialize mouse
int usb_mouse_init(usb_mouse_t* mouse) {
    if (!mouse) {
        return -1;
    }
    
    mouse->x = 0;
    mouse->y = 0;
    mouse->wheel = 0;
    mouse->buttons = 0;
    mouse->last_buttons = 0;
    
    printf("USB Mouse: Başlatıldı\n");
    return 0;
}

// Process mouse report
void usb_mouse_process_report(usb_mouse_t* mouse, const uint8_t* report) {
    if (!mouse || !report) {
        return;
    }
    
    // Standard mouse report format:
    // Byte 0: Buttons
    // Byte 1: X movement
    // Byte 2: Y movement
    // Byte 3: Wheel (optional)
    
    uint8_t new_buttons = report[0];
    int8_t x_movement = (int8_t)report[1];
    int8_t y_movement = (int8_t)report[2];
    int8_t wheel_movement = 0;
    
    if (mouse->base.max_packet_size > 3) {
        wheel_movement = (int8_t)report[3];
    }
    
    // Handle button changes
    uint8_t changed_buttons = new_buttons ^ mouse->last_buttons;
    if (changed_buttons) {
        for (uint8_t i = 0; i < 8; i++) {
            if (changed_buttons & (1 << i)) {
                uint8_t pressed = (new_buttons & (1 << i)) ? 1 : 0;
                
                if (mouse->button_handler) {
                    mouse->button_handler(i, pressed);
                }
                
                printf("USB Mouse: Button %d %s\n", i, pressed ? "basıldı" : "bırakıldı");
            }
        }
    }
    
    // Handle movement
    if (x_movement != 0 || y_movement != 0 || wheel_movement != 0) {
        mouse->x += x_movement;
        mouse->y += y_movement;
        mouse->wheel += wheel_movement;
        
        if (mouse->move_handler) {
            mouse->move_handler(x_movement, y_movement, wheel_movement);
        }
        
        printf("USB Mouse: Hareket X=%d, Y=%d, Wheel=%d (Toplam: X=%d, Y=%d, W=%d)\n",
               x_movement, y_movement, wheel_movement, mouse->x, mouse->y, mouse->wheel);
    }
    
    // Update state
    mouse->last_buttons = new_buttons;
}

// Probe USB device for HID support
int usb_hid_probe(usb_device_t* usb_device) {
    if (!usb_device) {
        return -1;
    }
    
    // Check if this is a HID device
    if (usb_device->device_desc.class_code != USB_HID_CLASS) {
        return -1;
    }
    
    printf("USB HID: Aygıt tespit edildi: VID:PID=%04X:%04X\n",
           usb_device->device_desc.vendor_id, usb_device->device_desc.product_id);
    
    // Create HID device structure
    usb_hid_device_t* device = malloc(sizeof(usb_hid_device_t));
    if (!device) {
        printf("USB HID: Bellek tahsis hatası\n");
        return -1;
    }
    
    memset(device, 0, sizeof(usb_hid_device_t));
    device->usb_device = usb_device;
    
    // TODO: Get HID descriptor from interface
    // For now, use default values
    device->hid_descriptor.length = 9;
    device->hid_descriptor.type = HID_DESC_TYPE_HID;
    device->hid_descriptor.bcd_hid = 0x0111;
    device->hid_descriptor.country_code = 0;
    device->hid_descriptor.num_descriptors = 1;
    device->hid_descriptor.descriptor_type = HID_DESC_TYPE_REPORT;
    device->hid_descriptor.descriptor_length = 64; // Default size
    
    // TODO: Find interrupt endpoint from interface descriptor
    // For now, use default values
    device->interrupt_in_endpoint = 0x81; // EP1 IN
    device->max_packet_size = 8;
    device->poll_interval = 10;
    
    // Allocate report descriptor buffer
    device->report_descriptor = malloc(device->hid_descriptor.descriptor_length);
    if (!device->report_descriptor) {
        printf("USB HID: Report descriptor bellek tahsis hatası\n");
        free(device);
        return -1;
    }
    
    // TODO: Get actual report descriptor
    // For now, use a minimal placeholder
    memset(device->report_descriptor, 0, device->hid_descriptor.descriptor_length);
    
    // Parse report descriptor
    usb_hid_parse_report_descriptor(device);
    
    // Set protocol to report protocol
    usb_hid_set_protocol(device, 1);
    
    // Set idle rate
    usb_hid_set_idle(device, 0, 0);
    
    device->initialized = 1;
    
    // Add to device list
    device->next = hid_devices;
    hid_devices = device;
    hid_driver.device_count++;
    
    printf("USB HID: Aygıt eklendi (Tip: %d)\n", device->hid_type);
    
    // Create specific device structures based on type
    if (device->hid_type == 1) { // Keyboard
        usb_keyboard_t* keyboard = malloc(sizeof(usb_keyboard_t));
        if (keyboard) {
            memset(keyboard, 0, sizeof(usb_keyboard_t));
            memcpy(&keyboard->base, device, sizeof(usb_hid_device_t));
            usb_keyboard_init(keyboard);
        }
    } else if (device->hid_type == 2) { // Mouse
        usb_mouse_t* mouse = malloc(sizeof(usb_mouse_t));
        if (mouse) {
            memset(mouse, 0, sizeof(usb_mouse_t));
            memcpy(&mouse->base, device, sizeof(usb_hid_device_t));
            usb_mouse_init(mouse);
        }
    }
    
    return 0;
}

// Remove HID device
int usb_hid_remove(usb_device_t* usb_device) {
    usb_hid_device_t** current = &hid_devices;
    
    while (*current) {
        if ((*current)->usb_device == usb_device) {
            usb_hid_device_t* to_remove = *current;
            *current = (*current)->next;
            hid_driver.device_count--;
            
            printf("USB HID: Aygıt kaldırıldı (Tip: %d)\n", to_remove->hid_type);
            
            if (to_remove->report_descriptor) {
                free(to_remove->report_descriptor);
            }
            
            free(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;
}

// List all HID devices
void usb_hid_list_devices() {
    printf("\n=== USB HID Aygıtları ===\n");
    
    usb_hid_device_t* current = hid_devices;
    int count = 1;
    
    while (current) {
        const char* type_str = "Bilinmeyen";
        switch (current->hid_type) {
            case 1: type_str = "Klavye"; break;
            case 2: type_str = "Fare"; break;
            case 3: type_str = "Diğer"; break;
        }
        
        printf("%d. %s %04X:%04X (%s)\n", count++,
               current->usb_device->device_desc.vendor_id ? "VID:PID" : "Aygıt",
               current->usb_device->device_desc.vendor_id,
               current->usb_device->device_desc.product_id,
               type_str);
        
        printf("   Endpoint: 0x%02X, Max Packet: %d, Poll: %dms\n",
               current->interrupt_in_endpoint, current->max_packet_size, current->poll_interval);
        
        current = current->next;
    }
    
    if (count == 1) {
        printf("Hiçbir USB HID aygıtı bağlı değil\n");
    }
    
    printf("========================\n");
}

// Find device by USB device
usb_hid_device_t* usb_hid_find_device(usb_device_t* usb_device) {
    usb_hid_device_t* current = hid_devices;
    
    while (current) {
        if (current->usb_device == usb_device) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Convert key to scancode
uint8_t usb_key_to_scancode(uint8_t key) {
    // Simple mapping - in a real implementation this would be more comprehensive
    return key;
}

// Convert key to string
const char* usb_key_to_string(uint8_t key) {
    if (key < sizeof(key_names)/sizeof(key_names[0])) {
        return key_names[key];
    }
    return "Unknown";
}

// Create USB HID driver
driver_t* create_usb_hid_driver() {
    if (usb_hid_init() != 0) {
        return NULL;
    }
    
    return (driver_t*)&hid_driver;
}
