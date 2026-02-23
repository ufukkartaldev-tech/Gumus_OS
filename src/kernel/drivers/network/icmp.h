#ifndef ICMP_H
#define ICMP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"

// ICMP Tipleri
#define ICMP_TYPE_ECHO_REPLY       0
#define ICMP_TYPE_DEST_UNREACHABLE 3
#define ICMP_TYPE_ECHO_REQUEST     8
#define ICMP_TYPE_TIME_EXCEEDED    11

// ICMP KodlarÄ±
#define ICMP_CODE_NET_UNREACHABLE  0
#define ICMP_CODE_HOST_UNREACHABLE 1
#define ICMP_CODE_PORT_UNREACHABLE 3

// ICMP Header
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t rest_of_header; // Type ve code'a gÃ¶re deÄŸiÅŸir
} __attribute__((packed)) icmp_header_t;

// ICMP Echo Request/Reply
typedef struct {
    icmp_header_t header;
    uint16_t identifier;
    uint16_t sequence_number;
    uint8_t data[64]; // VarsayÄ±lan veri
} __attribute__((packed)) icmp_echo_t;

// ICMP FonksiyonlarÄ±
int icmp_init();
int icmp_send_echo(uint8_t* dest_ip, uint16_t identifier, uint16_t sequence);
int icmp_send_echo_reply(uint8_t* dest_ip, uint16_t identifier, uint16_t sequence);
int icmp_process_packet(icmp_header_t* packet, uint32_t size, uint8_t* source_ip);
uint16_t icmp_calculate_checksum(void* data, uint32_t size);
void icmp_print_packet(icmp_header_t* packet, uint32_t size);

// Ping Utility FonksiyonlarÄ±
int ping(uint8_t* dest_ip, uint32_t count);
void ping_start(uint8_t* dest_ip);
void ping_stop();

#endif
