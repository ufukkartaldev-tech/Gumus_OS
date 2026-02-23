#ifndef USB_DRIVER_MANAGER_H
#define USB_DRIVER_MANAGER_H

#include "usb_host.h"
#include "usb_mass_storage.h"
#include "usb_hid_class.h"
#include "usb_audio.h"

// USB Driver Manager Functions
void usb_drivers_init();
void usb_drivers_cleanup();
void usb_drivers_status();
void usb_drivers_test();

// USB Device Class Detection
void usb_device_class_probe(usb_device_t* usb_device);
void usb_device_remove_notify(usb_device_t* usb_device);

// USB Hot-plug Management
void usb_hotplug_event_handler(uint8_t controller_type, uint8_t port, uint8_t connected);

// USB System Management
int usb_system_init();
void usb_system_shutdown();

#endif
