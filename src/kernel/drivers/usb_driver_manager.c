#include "usb_host.h"
#include "usb_mass_storage.h"
#include "usb_hid_class.h"
#include "usb_audio.h"
#include "driver_manager.h"
#include "hardware_detect.h"
#include "stdio.h"

// USB driver initialization function
void usb_drivers_init() {
    printf("USB Drivers: Başlatılıyor\n");
    
    // Initialize USB host system
    usb_host_init();
    
    // Register USB class drivers
    driver_t* mass_storage_driver = create_usb_mass_storage_driver();
    if (mass_storage_driver) {
        driver_register(mass_storage_driver);
        printf("USB Drivers: Mass Storage sürücüsü kaydedildi\n");
    }
    
    driver_t* hid_driver = create_usb_hid_driver();
    if (hid_driver) {
        driver_register(hid_driver);
        printf("USB Drivers: HID sürücüsü kaydedildi\n");
    }
    
    driver_t* audio_driver = create_usb_audio_driver();
    if (audio_driver) {
        driver_register(audio_driver);
        printf("USB Drivers: Audio sürücüsü kaydedildi\n");
    }
    
    printf("USB Drivers: Başlatıldı\n");
}

// USB device class detection and driver assignment
void usb_device_class_probe(usb_device_t* usb_device) {
    if (!usb_device) {
        return;
    }
    
    printf("USB Drivers: Aygıt sınıfı tespit ediliyor: %04X:%04X (Class: %02X)\n",
           usb_device->device_desc.vendor_id, usb_device->device_desc.product_id,
           usb_device->device_desc.class_code);
    
    // Probe device with appropriate class driver
    switch (usb_device->device_desc.class_code) {
        case USB_DEVICE_TYPE_STORAGE:
            printf("USB Drivers: Mass Storage aygıtı tespit edildi\n");
            usb_mass_storage_probe(usb_device);
            break;
            
        case USB_DEVICE_TYPE_HID:
            printf("USB Drivers: HID aygıtı tespit edildi\n");
            usb_hid_probe(usb_device);
            break;
            
        case USB_DEVICE_TYPE_AUDIO:
            printf("USB Drivers: Audio aygıtı tespit edildi\n");
            usb_audio_probe(usb_device);
            break;
            
        case USB_DEVICE_TYPE_HUB:
            printf("USB Drivers: Hub aygıtı tespit edildi (henüz desteklenmiyor)\n");
            break;
            
        default:
            printf("USB Drivers: Bilinmeyen aygıt sınıfı: %02X\n", 
                   usb_device->device_desc.class_code);
            
            // Try to detect by interface class
            if (usb_device->interface_desc.class_code == USB_DEVICE_TYPE_HID) {
                printf("USB Drivers: HID aygıtı (interface bazında) tespit edildi\n");
                usb_hid_probe(usb_device);
            } else if (usb_device->interface_desc.class_code == USB_DEVICE_TYPE_AUDIO) {
                printf("USB Drivers: Audio aygıtı (interface bazında) tespit edildi\n");
                usb_audio_probe(usb_device);
            }
            break;
    }
}

// USB device removal notification
void usb_device_remove_notify(usb_device_t* usb_device) {
    if (!usb_device) {
        return;
    }
    
    printf("USB Drivers: Aygıt kaldırılıyor: %04X:%04X\n",
           usb_device->device_desc.vendor_id, usb_device->device_desc.product_id);
    
    // Remove device from all class drivers
    usb_mass_storage_remove(usb_device);
    usb_hid_remove(usb_device);
    usb_audio_remove(usb_device);
    
    // Remove from USB host system
    usb_host_remove_device(usb_device->id);
}

// USB driver status reporting
void usb_drivers_status() {
    printf("\n=== USB Driver Durumu ===\n");
    
    // USB Host Controllers
    usb_host_list_controllers();
    
    // USB Devices
    usb_host_list_devices();
    
    // Mass Storage Devices
    usb_mass_storage_list_devices();
    
    // HID Devices
    usb_hid_list_devices();
    
    // Audio Devices
    usb_audio_list_devices();
    
    printf("========================\n");
}

// USB hot-plug event handler
void usb_hotplug_event_handler(uint8_t controller_type, uint8_t port, uint8_t connected) {
    printf("USB Drivers: Hot-plug olayı - Controller: %d, Port: %d, %s\n",
           controller_type, port, connected ? "bağlandı" : "kaldırıldı");
    
    if (connected) {
        // Device connected - try to enumerate
        usb_host_controller_t* controller = usb_host_find_controller(controller_type);
        if (controller) {
            usb_host_enumerate_device(controller, port);
        }
    } else {
        // Device disconnected - find and remove
        // This would require tracking which device is on which port
        // For now, just notify all drivers
        usb_host_check_hotplug();
    }
}

// USB driver cleanup
void usb_drivers_cleanup() {
    printf("USB Drivers: Temizleniyor\n");
    
    // Remove all devices
    usb_mass_storage_remove(NULL);
    usb_hid_remove(NULL);
    usb_audio_remove(NULL);
    
    // Unregister drivers
    driver_unregister("USB Mass Storage Driver");
    driver_unregister("USB HID Driver");
    driver_unregister("USB Audio Driver");
    
    printf("USB Drivers: Temizlendi\n");
}

// USB driver test function
void usb_drivers_test() {
    printf("USB Drivers: Test başlatılıyor\n");
    
    // Test USB host system
    printf("USB Host Controller sayısı: %d\n", usb_host_count_controllers());
    printf("USB Aygıt sayısı: %d\n", usb_host_count_devices());
    
    // Test device enumeration
    usb_host_check_hotplug();
    
    // Show status
    usb_drivers_status();
    
    printf("USB Drivers: Test tamamlandı\n");
}

// USB driver initialization for system startup
int usb_system_init() {
    printf("USB System: Başlatılıyor\n");
    
    // Initialize driver manager
    driver_manager_init();
    
    // Initialize USB drivers
    usb_drivers_init();
    
    // Auto-detect and load USB controllers
    driver_auto_load_all();
    
    // Test the system
    usb_drivers_test();
    
    printf("USB System: Başlatıldı\n");
    return 0;
}

// USB driver shutdown for system shutdown
void usb_system_shutdown() {
    printf("USB System: Kapatılıyor\n");
    
    // Cleanup USB drivers
    usb_drivers_cleanup();
    
    // Cleanup driver manager
    driver_manager_cleanup();
    
    printf("USB System: Kapatıldı\n");
}
