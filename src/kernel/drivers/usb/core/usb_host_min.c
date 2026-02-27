#include "usb_host.h"

void usb_host_init() {}
int usb_host_register_controller(usb_host_controller_t* controller) { (void)controller; return 0; }
int usb_host_unregister_controller(usb_host_controller_t* controller) { (void)controller; return 0; }
int usb_host_enumerate_device(usb_host_controller_t* controller, uint8_t port) { (void)controller; (void)port; return 0; }
int usb_host_control_transfer(usb_host_controller_t* controller, uint8_t device_addr, uint8_t endpoint, usb_setup_packet_t* setup, uint8_t* data, uint32_t length) {
    (void)controller; (void)device_addr; (void)endpoint; (void)setup; (void)data; (void)length; return 0;
}
int usb_host_bulk_transfer(usb_host_controller_t* controller, uint8_t device_addr, uint8_t endpoint, uint8_t* data, uint32_t length, uint8_t direction) {
    (void)controller; (void)device_addr; (void)endpoint; (void)data; (void)length; (void)direction; return 0;
}
int usb_host_interrupt_transfer(usb_host_controller_t* controller, uint8_t device_addr, uint8_t endpoint, uint8_t* data, uint32_t length) {
    (void)controller; (void)device_addr; (void)endpoint; (void)data; (void)length; return 0;
}
void usb_host_list_devices() {}
void usb_host_list_controllers() {}

