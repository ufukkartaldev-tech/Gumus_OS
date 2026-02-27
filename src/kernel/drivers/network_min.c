#include "network_driver.h"
#include "driver.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Minimal implementations for network layer to satisfy linking in minimal builds

int rtl8139_init(network_driver_t* driver) { if (driver) driver->initialized = 1; return 0; }
int rtl8139_send(network_driver_t* driver, uint8_t* data, uint32_t size) { (void)driver; (void)data; (void)size; return 0; }
int rtl8139_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size) { (void)driver; (void)buffer; (void)size; return -1; }
int rtl8139_set_mac(network_driver_t* driver, uint8_t* mac) { if (!driver||!mac) return -1; memcpy(driver->mac_address, mac, 6); return 0; }
int rtl8139_get_status(network_driver_t* driver) { (void)driver; return 0; }
int rtl8139_reset(network_driver_t* driver) { (void)driver; return 0; }

int e1000_init(network_driver_t* driver) { if (driver) driver->initialized = 1; return 0; }
int e1000_send(network_driver_t* driver, uint8_t* data, uint32_t size) { (void)driver; (void)data; (void)size; return 0; }
int e1000_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size) { (void)driver; (void)buffer; (void)size; return -1; }
int e1000_set_mac(network_driver_t* driver, uint8_t* mac) { if (!driver||!mac) return -1; memcpy(driver->mac_address, mac, 6); return 0; }
int e1000_get_status(network_driver_t* driver) { (void)driver; return 0; }
int e1000_reset(network_driver_t* driver) { (void)driver; return 0; }

uint16_t network_checksum(uint8_t* data, uint32_t size) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i + 1 < size; i += 2) {
        sum += (data[i] << 8) | data[i+1];
        if (sum & 0x10000) sum = (sum & 0xFFFF) + 1;
    }
    if (size & 1) {
        sum += data[size-1] << 8;
        if (sum & 0x10000) sum = (sum & 0xFFFF) + 1;
    }
    return (uint16_t)~sum;
}

void network_format_mac(uint8_t* mac, char* buffer) {
    static const char* hex = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) {
        buffer[i*3+0] = hex[(mac[i] >> 4) & 0xF];
        buffer[i*3+1] = hex[mac[i] & 0xF];
        buffer[i*3+2] = (i == 5) ? '\0' : ':';
    }
}

void network_format_ip(uint32_t ip, char* buffer) {
    uint8_t* b = (uint8_t*)&ip;
    // Assume little-endian host; present in dotted decimal
    // ip stored as host order here
    sprintf(buffer, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
}

uint32_t network_parse_ip(const char* ip_string) {
    uint32_t a=0,b=0,c=0,d=0;
    const char* p = ip_string;
    a = (uint32_t)atoi(p);
    while (*p && *p != '.') p++; if (*p) p++;
    b = (uint32_t)atoi(p);
    while (*p && *p != '.') p++; if (*p) p++;
    c = (uint32_t)atoi(p);
    while (*p && *p != '.') p++; if (*p) p++;
    d = (uint32_t)atoi(p);
    return (a & 0xFF) | ((b & 0xFF) << 8) | ((c & 0xFF) << 16) | ((d & 0xFF) << 24);
}

int network_mac_equal(uint8_t* mac1, uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

void network_copy_mac(uint8_t* dest, uint8_t* src) {
    memcpy(dest, src, 6);
}

driver_t* create_rtl8139_driver(pci_device_t* device) { (void)device; return 0; }
driver_t* create_e1000_driver(pci_device_t* device) { (void)device; return 0; }
