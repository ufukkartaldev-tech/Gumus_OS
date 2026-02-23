#ifndef IP_H
#define IP_H

#include <stdint.h>
#include <stddef.h>

#define htons(n) ((((uint16_t)(n) & 0xFF00) >> 8) | (((uint16_t)(n) & 0x00FF) << 8))
#define ntohs(n) htons(n)
#define htonl(n) ((((uint32_t)(n) & 0x000000FF) << 24) | (((uint32_t)(n) & 0x0000FF00) << 8) | (((uint32_t)(n) & 0x00FF0000) >> 8) | (((uint32_t)(n) & 0xFF000000) >> 24))
#define ntohl(n) htonl(n)

#include "ethernet.h"
#include "arp.h"

// IP Sabitleri
#define IP_VERSION_4            4
#define IP_HEADER_LENGTH        5
#define IP_PROTOCOL_ICMP        1
#define IP_PROTOCOL_TCP         6
#define IP_PROTOCOL_UDP         17
#define IP_MAX_TTL              255
#define IP_MAX_PACKET_SIZE      65535

// IP Header
typedef struct {
    uint8_t version_ihl;        // Version (4 bits) + IHL (4 bits)
    uint8_t dscp_ecn;           // DSCP (6 bits) + ECN (2 bits)
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;    // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_checksum;
    uint8_t source_ip[4];
    uint8_t dest_ip[4];
} __attribute__((packed)) ip_header_t;

// IP Packet
typedef struct {
    ip_header_t header;
    uint8_t data[IP_MAX_PACKET_SIZE - sizeof(ip_header_t)];
} __attribute__((packed)) ip_packet_t;

// IP FonksiyonlarÄ±
int ip_init();
int ip_send_packet(uint8_t* dest_ip, uint8_t protocol, void* data, uint32_t size);
int ip_receive_packet(ip_packet_t* packet);
int ip_process_packet(ip_packet_t* packet, uint32_t size);
uint16_t ip_calculate_checksum(void* data, uint32_t size);
void ip_set_source_ip(uint8_t* ip);
uint8_t* ip_get_source_ip();
void ip_print_packet(ip_packet_t* packet);

// IP Fragmentasyon FonksiyonlarÄ±
int ip_fragment_packet(uint8_t* dest_ip, uint8_t protocol, void* data, uint32_t size);
int ip_reassemble_packet(ip_packet_t* fragment, uint32_t size);

// IP Routing
int ip_route_packet(uint8_t* dest_ip, mac_addr_t* next_hop_mac);

// Pseudo-header for checksum (Shared by TCP/UDP)
typedef struct {
    uint8_t source_ip[4];
    uint8_t dest_ip[4];
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_length;
} __attribute__((packed)) udp_pseudo_header_t;

#endif
