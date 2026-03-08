#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"
#include "udp.h"

// DHCP Sabitleri
#define DHCP_SERVER_PORT         67
#define DHCP_CLIENT_PORT         68
#define DHCP_MAGIC_COOKIE        0x63825363
#define DHCP_BROADCAST_FLAG      0x8000

// DHCP Mesaj Türleri
#define DHCP_DISCOVER            1
#define DHCP_OFFER               2
#define DHCP_REQUEST             3
#define DHCP_DECLINE             4
#define DHCP_ACK                 5
#define DHCP_NAK                 6
#define DHCP_RELEASE              7
#define DHCP_INFORM              8

// DHCP Seçenekleri
#define DHCP_OPTION_SUBNET_MASK     1
#define DHCP_OPTION_ROUTER           3
#define DHCP_OPTION_DNS_SERVER       6
#define DHCP_OPTION_REQUESTED_IP     50
#define DHCP_OPTION_LEASE_TIME       51
#define DHCP_OPTION_MESSAGE_TYPE     53
#define DHCP_OPTION_SERVER_ID        54
#define DHCP_OPTION_PARAM_REQUEST    55
#define DHCP_OPTION_END              255

// DHCP Header
typedef struct {
    uint8_t op;                // Message op code / message type
    uint8_t htype;             // Hardware address type
    uint8_t hlen;              // Hardware address length
    uint8_t hops;              // Hops
    uint32_t xid;              // Transaction ID
    uint16_t secs;             // Seconds elapsed
    uint16_t flags;            // Bootp flags
    uint32_t ciaddr;           // Client IP address
    uint32_t yiaddr;           // 'your' (client) IP address
    uint32_t siaddr;           // IP address of next server
    uint32_t giaddr;           // Relay agent IP address
    uint8_t chaddr[16];        // Client hardware address
    uint8_t sname[64];         // Optional server host name
    uint8_t file[128];         // Boot file name
    uint32_t magic_cookie;     // DHCP magic cookie
    uint8_t options[312];      // Optional parameters field
} __attribute__((packed)) dhcp_packet_t;

// DHCP Seçeneği
typedef struct {
    uint8_t code;
    uint8_t length;
    uint8_t data[];
} __attribute__((packed)) dhcp_option_t;

// DHCP Interface Yapısı
typedef struct {
    uint8_t ip[4];             // Interface IP adresi
    uint8_t netmask[4];        // Subnet mask
    uint8_t gateway[4];        // Gateway IP
    uint8_t dns_server[4];     // DNS server
    uint8_t dhcp_server[4];    // DHCP server IP
    uint32_t lease_time;       // Lease time in seconds
    uint32_t renewal_time;     // Renewal time
    uint32_t rebinding_time;   // Rebinding time
    uint8_t configured;        // Interface konfigüre edildi mi?
} dhcp_interface_t;

// DHCP Durumları
typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING,
    DHCP_STATE_INIT_REBOOT
} dhcp_state_t;

// DHCP Client Yapısı
typedef struct {
    dhcp_interface_t interface;
    dhcp_state_t state;
    uint32_t transaction_id;
    uint32_t last_activity;
    uint32_t retry_count;
    uint8_t server_mac[6];
} dhcp_client_t;

// DHCP Fonksiyonları
int dhcp_init();
int dhcp_discover();
int dhcp_request();
int dhcp_release();
int dhcp_renew();
int dhcp_process_packet(dhcp_packet_t* packet, uint32_t size);
int dhcp_send_packet(uint8_t message_type, uint32_t requested_ip);
int dhcp_add_option(uint8_t* options, uint8_t code, uint8_t length, void* data);
uint8_t* dhcp_find_option(dhcp_packet_t* packet, uint8_t code);
void dhcp_print_config();
int dhcp_configure_interface();

#endif
