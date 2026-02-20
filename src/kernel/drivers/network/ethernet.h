#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include <stddef.h>
#include "../driver.h"

// Ethernet Sabitleri
#define ETHERNET_ADDR_LEN      6
#define ETHERNET_TYPE_LEN      2
#define ETHERNET_HEADER_SIZE   14
#define ETHERNET_MAX_PAYLOAD   1500
#define ETHERNET_FRAME_SIZE    (ETHERNET_HEADER_SIZE + ETHERNET_MAX_PAYLOAD)

// Ethernet Tipleri
#define ETHERNET_TYPE_IPV4     0x0800
#define ETHERNET_TYPE_ARP      0x0806
#define ETHERNET_TYPE_IPV6     0x86DD

// MAC Adresi Yapısı
typedef struct {
    uint8_t addr[ETHERNET_ADDR_LEN];
} mac_addr_t;

// Ethernet Header
typedef struct {
    mac_addr_t dest;
    mac_addr_t src;
    uint16_t type;
} __attribute__((packed)) ethernet_header_t;

// Ethernet Frame
typedef struct {
    ethernet_header_t header;
    uint8_t payload[ETHERNET_MAX_PAYLOAD];
} __attribute__((packed)) ethernet_frame_t;

// Ethernet Sürücü Fonksiyonları
int ethernet_init();
int ethernet_send_frame(mac_addr_t* dest, uint16_t type, void* data, uint32_t size);
int ethernet_receive_frame(ethernet_frame_t* frame);
void ethernet_set_mac(mac_addr_t* mac);
mac_addr_t ethernet_get_mac();
void ethernet_print_mac(mac_addr_t* mac);

// MAC Adresi Utility Fonksiyonları
int ethernet_mac_equal(mac_addr_t* a, mac_addr_t* b);
int ethernet_mac_is_broadcast(mac_addr_t* mac);
int ethernet_mac_is_multicast(mac_addr_t* mac);
void ethernet_copy_mac(mac_addr_t* dest, mac_addr_t* src);

// Ethernet Sürücü Yapısı
extern driver_t ethernet_driver;

#endif
