#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"
#include "udp.h"

// DNS Sabitleri
#define DNS_PORT                 53
#define DNS_MAX_PACKET_SIZE      512
#define DNS_MAX_NAME_LENGTH      255
#define DNS_MAX_LABELS           63
#define DNS_CACHE_SIZE           64
#define DNS_CACHE_TTL            300  // 5 dakika

// DNS Record Types
#define DNS_TYPE_A               1    // IPv4 address
#define DNS_TYPE_AAAA            28   // IPv6 address
#define DNS_TYPE_CNAME           5    // Canonical name
#define DNS_TYPE_MX              15   // Mail exchange
#define DNS_TYPE_NS              2    // Name server
#define DNS_TYPE_PTR             12   // Pointer
#define DNS_TYPE_TXT             16   // Text

// DNS Record Classes
#define DNS_CLASS_IN             1    // Internet

// DNS Response Codes
#define DNS_RESPONSE_NO_ERROR     0
#define DNS_RESPONSE_FORM_ERR     1
#define DNS_RESPONSE_SERV_FAIL    2
#define DNS_RESPONSE_NX_DOMAIN    3
#define DNS_RESPONSE_NOT_IMPL     4
#define DNS_RESPONSE_REFUSED      5

// DNS Header
typedef struct {
    uint16_t id;                // Transaction ID
    uint16_t flags;             // Flags (QR, Opcode, AA, TC, RD, RA, Z, RCODE)
    uint16_t qdcount;           // Question count
    uint16_t ancount;           // Answer count
    uint16_t nscount;           // Authority count
    uint16_t arcount;           // Additional count
} __attribute__((packed)) dns_header_t;

// DNS Question
typedef struct {
    uint8_t qname[DNS_MAX_NAME_LENGTH];  // Query name
    uint16_t qtype;                       // Query type
    uint16_t qclass;                      // Query class
} __attribute__((packed)) dns_question_t;

// DNS Record
typedef struct {
    uint8_t name[DNS_MAX_NAME_LENGTH];   // Record name (compressed)
    uint16_t type;                        // Record type
    uint16_t class;                       // Record class
    uint32_t ttl;                         // Time to live
    uint16_t rdlength;                    // Data length
    uint8_t rdata[256];                   // Record data
} __attribute__((packed)) dns_record_t;

// DNS Packet
typedef struct {
    dns_header_t header;
    dns_question_t questions[10];
    dns_record_t answers[10];
    dns_record_t authority[10];
    dns_record_t additional[10];
} __attribute__((packed)) dns_packet_t;

// DNS Cache Entry
typedef struct {
    char domain[DNS_MAX_NAME_LENGTH];
    uint8_t ip[4];               // IPv4 address
    uint32_t ttl;                // Time to live
    uint32_t timestamp;          // Cache entry timestamp
    uint8_t valid;               // Entry is valid
} dns_cache_entry_t;

// DNS Resolver Structure
typedef struct {
    uint8_t dns_server[4];       // DNS server IP
    dns_cache_entry_t cache[DNS_CACHE_SIZE];
    uint16_t transaction_id;
    uint32_t timeout;
} dns_resolver_t;

// DNS Fonksiyonları
int dns_init();
int dns_resolve(const char* domain, uint8_t* ip);
int dns_query(const char* domain, uint16_t type, dns_packet_t* response);
int dns_send_query(const char* domain, uint16_t type);
int dns_process_response(dns_packet_t* packet, uint32_t size);
int dns_encode_name(const char* domain, uint8_t* buffer);
int dns_decode_name(const uint8_t* buffer, const uint8_t* start, char* domain);
int dns_add_cache_entry(const char* domain, uint8_t* ip, uint32_t ttl);
int dns_lookup_cache(const char* domain, uint8_t* ip);
void dns_cache_cleanup();
int dns_parse_record(const uint8_t* buffer, uint32_t* offset, dns_record_t* record);
void dns_print_cache();
int dns_set_server(uint8_t* server_ip);

// DNS Utility Functions
uint16_t dns_generate_id();
int dns_is_valid_domain(const char* domain);
void dns_ip_to_string(uint8_t* ip, char* str);
int dns_string_to_ip(const char* str, uint8_t* ip);

#endif
