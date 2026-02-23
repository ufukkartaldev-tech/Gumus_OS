#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"

// TCP Header
typedef struct {
    uint16_t source_port;
    uint16_t dest_port;
    uint32_t sequence_number;
    uint32_t ack_number;
    uint16_t flags; // Data offset (4) + Reserved (3) + Flags (9)
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__((packed)) tcp_header_t;

// TCP Flags
#define TCP_FLAG_FIN (1 << 0)
#define TCP_FLAG_SYN (1 << 1)
#define TCP_FLAG_RST (1 << 2)
#define TCP_FLAG_PSH (1 << 3)
#define TCP_FLAG_ACK (1 << 4)
#define TCP_FLAG_URG (1 << 5)
#define TCP_FLAG_ECE (1 << 6)
#define TCP_FLAG_CWR (1 << 7)
#define TCP_FLAG_NS  (1 << 8)

// TCP States
typedef enum {
    TCP_STATE_CLOSED,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT1,
    TCP_STATE_FIN_WAIT2,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_TIME_WAIT
} tcp_state_t;

// TCP Connection
typedef struct {
    uint8_t remote_ip[4];
    uint16_t local_port;
    uint16_t remote_port;
    tcp_state_t state;
    uint32_t seq;
    uint32_t ack;
} tcp_connection_t;

// TCP Functions
int tcp_init();
int tcp_send(tcp_connection_t* conn, void* data, uint32_t size, uint16_t flags);
int tcp_process_packet(ip_packet_t* ip_packet, uint32_t size);
uint16_t tcp_calculate_checksum(tcp_header_t* header, uint8_t* src_ip, uint8_t* dst_ip, void* data, uint32_t size);

#endif
