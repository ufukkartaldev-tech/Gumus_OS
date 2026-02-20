#ifndef ARP_H
#define ARP_H

#include <stdint.h>
#include <stddef.h>
#include "ethernet.h"

// ARP Sabitleri
#define ARP_HARDWARE_TYPE_ETHERNET  1
#define ARP_PROTOCOL_TYPE_IPV4      0x0800
#define ARP_OPERATION_REQUEST       1
#define ARP_OPERATION_REPLY         2
#define ARP_HW_ADDR_LEN             6
#define ARP_PROTO_ADDR_LEN          4

// ARP Header
typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hw_addr_len;
    uint8_t proto_addr_len;
    uint16_t operation;
    mac_addr_t sender_hw_addr;
    uint8_t sender_proto_addr[4];
    mac_addr_t target_hw_addr;
    uint8_t target_proto_addr[4];
} __attribute__((packed)) arp_packet_t;

// ARP Cache Entry
typedef struct arp_cache_entry {
    uint8_t ip_addr[4];
    mac_addr_t mac_addr;
    uint32_t timeout;
    struct arp_cache_entry* next;
} arp_cache_entry_t;

// ARP Fonksiyonları
int arp_init();
int arp_send_request(uint8_t* target_ip);
int arp_send_reply(uint8_t* target_ip, mac_addr_t* target_mac);
int arp_process_packet(arp_packet_t* packet, mac_addr_t* sender_mac);
int arp_resolve(uint8_t* ip_addr, mac_addr_t* mac_addr);
void arp_cache_add(uint8_t* ip_addr, mac_addr_t* mac_addr);
int arp_cache_lookup(uint8_t* ip_addr, mac_addr_t* mac_addr);
void arp_cache_cleanup();
void arp_print_packet(arp_packet_t* packet);

// IP Adresi Utility Fonksiyonları
void ip_to_string(uint8_t* ip, char* str);
void string_to_ip(char* str, uint8_t* ip);
int ip_equal(uint8_t* a, uint8_t* b);

#endif
