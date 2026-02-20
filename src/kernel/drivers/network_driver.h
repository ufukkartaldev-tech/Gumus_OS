#ifndef NETWORK_DRIVER_H
#define NETWORK_DRIVER_H

#include <stdint.h>
#include "driver.h"
#include "hardware_detect.h"

// Network Constants
#define NETWORK_MAX_PACKET_SIZE    1514
#define NETWORK_HEADER_SIZE        14
#define NETWORK_ARP_SIZE           42
#define NETWORK_ICMP_SIZE         98
#define NETWORK_UDP_SIZE          42
#define NETWORK_TCP_SIZE          54

// Ethernet Frame Types
#define ETH_TYPE_IPv4             0x0800
#define ETH_TYPE_ARP              0x0806
#define ETH_TYPE_VLAN             0x8100
#define ETH_TYPE_IPv6             0x86DD
#define ETH_TYPE_PPPoE_DISCOVERY  0x8863
#define ETH_TYPE_PPPoE_SESSION    0x8864

// IP Protocol Types
#define IP_PROTO_ICMP             0x01
#define IP_PROTO_TCP              0x06
#define IP_PROTO_UDP              0x11

// ARP Operation Types
#define ARP_OP_REQUEST            0x0001
#define ARP_OP_REPLY              0x0002

// ICMP Types
#define ICMP_TYPE_ECHO_REPLY      0x00
#define ICMP_TYPE_ECHO_REQUEST    0x08

// Network Device Structure
typedef struct {
    uint8_t mac[6];
    uint32_t ip;
    uint32_t subnet_mask;
    uint32_t gateway;
    uint32_t dns;
} network_config_t;

// Ethernet Header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_header_t;

// IP Header
typedef struct {
    uint8_t  version_ihl;
    uint8_t  tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t header_checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

// ARP Packet
typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_len;
    uint8_t  protocol_len;
    uint16_t operation;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

// ICMP Header
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

// Network Packet Buffer
typedef struct {
    uint8_t data[NETWORK_MAX_PACKET_SIZE];
    uint32_t size;
    uint32_t received_time;
} network_packet_t;

// Network Statistics
typedef struct {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
    uint32_t errors;
    uint32_t dropped;
} network_stats_t;

// Network Driver Interface
typedef struct {
    driver_t base;
    network_config_t config;
    network_stats_t stats;
    network_packet_t rx_buffer[32];
    network_packet_t tx_buffer[16];
    int rx_head, rx_tail;
    int tx_head, tx_tail;
    int initialized;
    uint8_t mac_address[6];
} network_driver_t;

// Network Function Prototypes
int network_init(network_driver_t* driver);
int network_send_packet(network_driver_t* driver, uint8_t* data, uint32_t size);
int network_receive_packet(network_driver_t* driver, uint8_t* buffer, uint32_t* size);
int network_set_mac_address(network_driver_t* driver, uint8_t* mac);
int network_set_ip_config(network_driver_t* driver, uint32_t ip, uint32_t subnet_mask, uint32_t gateway);
int network_send_arp_request(network_driver_t* driver, uint32_t target_ip);
int network_send_arp_reply(network_driver_t* driver, uint32_t target_ip, uint8_t* target_mac);
int network_send_ping(network_driver_t* driver, uint32_t target_ip);
int network_process_packet(network_driver_t* driver, uint8_t* packet, uint32_t size);

// Hardware-specific driver interfaces
typedef struct {
    int (*init)(network_driver_t* driver);
    int (*send)(network_driver_t* driver, uint8_t* data, uint32_t size);
    int (*receive)(network_driver_t* driver, uint8_t* buffer, uint32_t* size);
    int (*set_mac)(network_driver_t* driver, uint8_t* mac);
    int (*get_status)(network_driver_t* driver);
    int (*reset)(network_driver_t* driver);
} network_hw_interface_t;

// Realtek RTL8139 Driver
typedef struct {
    network_driver_t base;
    uint32_t io_base;
    uint8_t* tx_buffers[4];
    uint8_t* rx_buffer;
    uint32_t rx_buffer_size;
    uint16_t current_tx_buffer;
    uint32_t capabilities;
} rtl8139_driver_t;

// Intel E1000 Driver
typedef struct {
    network_driver_t base;
    uint32_t mmio_base;
    uint8_t* tx_descs[16];
    uint8_t* rx_descs[32];
    uint32_t tx_head, tx_tail;
    uint32_t rx_head, rx_tail;
    uint32_t mac_addr_low;
    uint32_t mac_addr_high;
} e1000_driver_t;

// Realtek RTL8139 Functions
int rtl8139_init(network_driver_t* driver);
int rtl8139_send(network_driver_t* driver, uint8_t* data, uint32_t size);
int rtl8139_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size);
int rtl8139_set_mac(network_driver_t* driver, uint8_t* mac);
int rtl8139_get_status(network_driver_t* driver);
int rtl8139_reset(network_driver_t* driver);

// Intel E1000 Functions
int e1000_init(network_driver_t* driver);
int e1000_send(network_driver_t* driver, uint8_t* data, uint32_t size);
int e1000_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size);
int e1000_set_mac(network_driver_t* driver, uint8_t* mac);
int e1000_get_status(network_driver_t* driver);
int e1000_reset(network_driver_t* driver);

// Utility Functions
uint16_t network_checksum(uint8_t* data, uint32_t size);
void network_format_mac(uint8_t* mac, char* buffer);
void network_format_ip(uint32_t ip, char* buffer);
uint32_t network_parse_ip(const char* ip_string);
int network_mac_equal(uint8_t* mac1, uint8_t* mac2);
void network_copy_mac(uint8_t* dest, uint8_t* src);

// Driver Creation Functions
driver_t* create_rtl8139_driver(pci_device_t* device);
driver_t* create_e1000_driver(pci_device_t* device);

#endif
