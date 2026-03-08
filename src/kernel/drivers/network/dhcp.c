#include "dhcp.h"
#include "ip.h"
#include "udp.h"
#include "ethernet.h"
#include "string.h"
#include "memory.h"
#include "stdio.h"
#include "stdlib.h"
#include "printf.h"

// Global DHCP client
static dhcp_client_t dhcp_client;
static uint32_t dhcp_timer = 0;

// UDP pseudo header için checksum hesaplaması
typedef struct {
    uint8_t source_ip[4];
    uint8_t dest_ip[4];
    uint8_t zero;
    uint8_t protocol;
    uint16_t udp_length;
} __attribute__((packed)) udp_pseudo_header_t;

// Rastgele transaction ID oluştur
static uint32_t dhcp_generate_xid() {
    static uint32_t counter = 0x12345678;
    return counter++;
}

// DHCP seçeneği ekle
int dhcp_add_option(uint8_t* options, uint8_t code, uint8_t length, void* data) {
    static int offset = 0;
    
    if (offset + 2 + length > 312) {
        return -1; // Buffer dolu
    }
    
    options[offset] = code;
    options[offset + 1] = length;
    
    if (length > 0 && data != NULL) {
        memcpy(&options[offset + 2], data, length);
    }
    
    offset += 2 + length;
    return offset;
}

// DHCP seçeneği bul
uint8_t* dhcp_find_option(dhcp_packet_t* packet, uint8_t code) {
    uint8_t* options = packet->options;
    int offset = 0;
    
    while (offset < 312 && options[offset] != DHCP_OPTION_END) {
        if (options[offset] == code) {
            return &options[offset];
        }
        
        if (options[offset] == 0) { // Padding
            offset++;
            continue;
        }
        
        uint8_t length = options[offset + 1];
        offset += 2 + length;
    }
    
    return NULL;
}

// DHCP paketi gönder
int dhcp_send_packet(uint8_t message_type, uint32_t requested_ip) {
    dhcp_packet_t packet;
    uint8_t broadcast_ip[4] = {255, 255, 255, 255};
    uint8_t options_offset = 0;
    
    // DHCP packet'ı sıfırla
    memset(&packet, 0, sizeof(dhcp_packet_t));
    
    // Header alanlarını doldur
    packet.op = 1; // BOOTREQUEST
    packet.htype = 1; // Ethernet
    packet.hlen = 6; // MAC adresi uzunluğu
    packet.hops = 0;
    packet.xid = dhcp_client.transaction_id;
    packet.secs = 0;
    packet.flags = htons(DHCP_BROADCAST_FLAG); // Broadcast kullan
    packet.ciaddr = 0;
    packet.yiaddr = 0;
    packet.siaddr = 0;
    packet.giaddr = 0;
    
    // Client MAC adresini kopyala
    extern uint8_t get_mac_address(uint8_t* mac);
    get_mac_address(packet.chaddr);
    
    // Magic cookie
    packet.magic_cookie = htonl(DHCP_MAGIC_COOKIE);
    
    // Seçenekleri ekle
    options_offset = 0;
    uint8_t* options = packet.options;
    
    // Message type seçeneği
    dhcp_add_option(options, DHCP_OPTION_MESSAGE_TYPE, 1, &message_type);
    
    // Parametre isteği seçeneği
    uint8_t param_request[] = {
        DHCP_OPTION_SUBNET_MASK,
        DHCP_OPTION_ROUTER,
        DHCP_OPTION_DNS_SERVER,
        DHCP_OPTION_LEASE_TIME
    };
    dhcp_add_option(options, DHCP_OPTION_PARAM_REQUEST, sizeof(param_request), param_request);
    
    // İstenen IP (REQUEST için)
    if (message_type == DHCP_REQUEST && requested_ip != 0) {
        uint32_t req_ip = htonl(requested_ip);
        dhcp_add_option(options, DHCP_OPTION_REQUESTED_IP, 4, &req_ip);
    }
    
    // Server identifier (REQUEST için)
    if (message_type == DHCP_REQUEST && dhcp_client.interface.dhcp_server[0] != 0) {
        dhcp_add_option(options, DHCP_OPTION_SERVER_ID, 4, dhcp_client.interface.dhcp_server);
    }
    
    // Seçenekleri bitir
    dhcp_add_option(options, DHCP_OPTION_END, 0, NULL);
    
    // Paketi broadcast olarak gönder
    return udp_send(broadcast_ip, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &packet, sizeof(dhcp_packet_t));
}

// DHCP Discovery gönder
int dhcp_discover() {
    printf("DHCP Discover gönderiliyor...\n");
    
    dhcp_client.state = DHCP_STATE_SELECTING;
    dhcp_client.transaction_id = dhcp_generate_xid();
    dhcp_client.retry_count = 0;
    
    return dhcp_send_packet(DHCP_DISCOVER, 0);
}

// DHCP Request gönder
int dhcp_request() {
    printf("DHCP Request gönderiliyor...\n");
    
    dhcp_client.state = DHCP_STATE_REQUESTING;
    dhcp_client.transaction_id = dhcp_generate_xid();
    
    uint32_t requested_ip = 0;
    if (dhcp_client.interface.ip[0] != 0) {
        memcpy(&requested_ip, dhcp_client.interface.ip, 4);
        requested_ip = ntohl(requested_ip);
    }
    
    return dhcp_send_packet(DHCP_REQUEST, requested_ip);
}

// DHCP Release gönder
int dhcp_release() {
    printf("DHCP Release gönderiliyor...\n");
    
    if (dhcp_client.interface.ip[0] == 0) {
        printf("IP adresi atanmamış.\n");
        return -1;
    }
    
    return dhcp_send_packet(DHCP_RELEASE, 0);
}

// DHCP Renew (yenile)
int dhcp_renew() {
    printf("DHCP yenileniyor...\n");
    
    if (dhcp_client.interface.ip[0] == 0) {
        printf("IP adresi atanmamış, Discover başlatılıyor.\n");
        return dhcp_discover();
    }
    
    dhcp_client.state = DHCP_STATE_RENEWING;
    return dhcp_request();
}

// DHCP paketini işle
int dhcp_process_packet(dhcp_packet_t* packet, uint32_t size) {
    if (size < sizeof(dhcp_packet_t)) {
        return -1;
    }
    
    // Magic cookie kontrolü
    if (ntohl(packet->magic_cookie) != DHCP_MAGIC_COOKIE) {
        return -1;
    }
    
    // Transaction ID kontrolü
    if (ntohl(packet->xid) != dhcp_client.transaction_id) {
        return -1;
    }
    
    // Message type seçeneğini bul
    uint8_t* msg_type_opt = dhcp_find_option(packet, DHCP_OPTION_MESSAGE_TYPE);
    if (!msg_type_opt) {
        return -1;
    }
    
    uint8_t message_type = msg_type_opt[2];
    
    switch (message_type) {
        case DHCP_OFFER:
            printf("DHCP Offer alındı.\n");
            
            // IP adresini kaydet
            uint32_t offered_ip = ntohl(packet->yiaddr);
            memcpy(dhcp_client.interface.ip, &offered_ip, 4);
            
            // DHCP server IP'sini kaydet
            uint8_t* server_id_opt = dhcp_find_option(packet, DHCP_OPTION_SERVER_ID);
            if (server_id_opt) {
                memcpy(dhcp_client.interface.dhcp_server, &server_id_opt[2], 4);
            }
            
            // Request gönder
            dhcp_request();
            break;
            
        case DHCP_ACK:
            printf("DHCP ACK alındı.\n");
            
            // Seçenekleri işle
            uint8_t* subnet_opt = dhcp_find_option(packet, DHCP_OPTION_SUBNET_MASK);
            if (subnet_opt) {
                memcpy(dhcp_client.interface.netmask, &subnet_opt[2], 4);
            }
            
            uint8_t* router_opt = dhcp_find_option(packet, DHCP_OPTION_ROUTER);
            if (router_opt) {
                memcpy(dhcp_client.interface.gateway, &router_opt[2], 4);
            }
            
            uint8_t* dns_opt = dhcp_find_option(packet, DHCP_OPTION_DNS_SERVER);
            if (dns_opt) {
                memcpy(dhcp_client.interface.dns_server, &dns_opt[2], 4);
            }
            
            uint8_t* lease_opt = dhcp_find_option(packet, DHCP_OPTION_LEASE_TIME);
            if (lease_opt) {
                memcpy(&dhcp_client.interface.lease_time, &lease_opt[2], 4);
                dhcp_client.interface.lease_time = ntohl(dhcp_client.interface.lease_time);
                dhcp_client.interface.renewal_time = dhcp_client.interface.lease_time / 2;
                dhcp_client.interface.rebinding_time = dhcp_client.interface.lease_time * 7 / 8;
            }
            
            dhcp_client.interface.configured = 1;
            dhcp_client.state = DHCP_STATE_BOUND;
            dhcp_client.last_activity = dhcp_timer;
            
            printf("DHCP konfigürasyonu tamamlandı.\n");
            dhcp_print_config();
            break;
            
        case DHCP_NAK:
            printf("DHCP NAK alındı.\n");
            dhcp_client.state = DHCP_STATE_INIT;
            dhcp_discover();
            break;
            
        default:
            printf("Bilinmeyen DHCP mesajı: %d\n", message_type);
            break;
    }
    
    return 0;
}

// DHCP konfigürasyonunu yazdır
void dhcp_print_config() {
    printf("=== DHCP Konfigürasyonu ===\n");
    printf("IP Adresi: %d.%d.%d.%d\n", 
           dhcp_client.interface.ip[0], dhcp_client.interface.ip[1],
           dhcp_client.interface.ip[2], dhcp_client.interface.ip[3]);
    printf("Subnet Mask: %d.%d.%d.%d\n",
           dhcp_client.interface.netmask[0], dhcp_client.interface.netmask[1],
           dhcp_client.interface.netmask[2], dhcp_client.interface.netmask[3]);
    printf("Gateway: %d.%d.%d.%d\n",
           dhcp_client.interface.gateway[0], dhcp_client.interface.gateway[1],
           dhcp_client.interface.gateway[2], dhcp_client.interface.gateway[3]);
    printf("DNS Server: %d.%d.%d.%d\n",
           dhcp_client.interface.dns_server[0], dhcp_client.interface.dns_server[1],
           dhcp_client.interface.dns_server[2], dhcp_client.interface.dns_server[3]);
    printf("DHCP Server: %d.%d.%d.%d\n",
           dhcp_client.interface.dhcp_server[0], dhcp_client.interface.dhcp_server[1],
           dhcp_client.interface.dhcp_server[2], dhcp_client.interface.dhcp_server[3]);
    printf("Lease Time: %d saniye\n", dhcp_client.interface.lease_time);
    printf("===========================\n");
}

// Network interface'i konfigüre et
int dhcp_configure_interface() {
    if (!dhcp_client.interface.configured) {
        return -1;
    }
    
    // IP adresini network stack'e set et
    ip_set_source_ip(dhcp_client.interface.ip);
    
    printf("Network interface DHCP ile konfigüre edildi.\n");
    return 0;
}

// DHCP'yi başlat
int dhcp_init() {
    printf("DHCP Client başlatılıyor...\n");
    
    // DHCP client yapısını sıfırla
    memset(&dhcp_client, 0, sizeof(dhcp_client_t));
    dhcp_client.state = DHCP_STATE_INIT;
    dhcp_client.transaction_id = dhcp_generate_xid();
    
    printf("DHCP Client hazır.\n");
    return 0;
}

// DHCP timer tick (her saniye çağrılmalı)
void dhcp_tick() {
    dhcp_timer++;
    
    // Lease renewal kontrolü
    if (dhcp_client.state == DHCP_STATE_BOUND && dhcp_client.interface.configured) {
        uint32_t elapsed = dhcp_timer - dhcp_client.last_activity;
        
        if (elapsed >= dhcp_client.interface.renewal_time) {
            printf("DHCP lease yenileme zamanı geldi.\n");
            dhcp_renew();
        }
    }
    
    // Retry kontrolü
    if (dhcp_client.retry_count > 0 && dhcp_client.retry_count < 5) {
        // 5 saniye bekle ve tekrar dene
        if (dhcp_timer % 5 == 0) {
            switch (dhcp_client.state) {
                case DHCP_STATE_SELECTING:
                    dhcp_discover();
                    break;
                case DHCP_STATE_REQUESTING:
                    dhcp_request();
                    break;
            }
            dhcp_client.retry_count++;
        }
    }
}
