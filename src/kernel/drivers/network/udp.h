#ifndef UDP_H
#define UDP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"

// UDP Header
typedef struct {
    uint16_t source_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// UDP Packet
typedef struct {
    udp_header_t header;
    uint8_t data[];
} __attribute__((packed)) udp_packet_t;

// UDP Functions
int udp_init();
int udp_send(uint8_t* dest_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t size);
int udp_process_packet(ip_packet_t* ip_packet, uint32_t size);
uint16_t udp_calculate_checksum(udp_header_t* header, uint8_t* src_ip, uint8_t* dst_ip, void* data, uint32_t size);

#endif
