#include "ip.h"
#include "udp.h"
#include "tcp.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "printf.h"
#include "stdio.h"
#include "stdlib.h"

// Global DeÄŸiÅŸkenler
static uint8_t source_ip[4] = {192, 168, 1, 100}; // VarsayÄ±lan IP
static int ip_initialized = 0;
static uint16_t packet_id = 1;

// Checksum Hesaplama
uint16_t ip_calculate_checksum(void* data, uint32_t size) {
    uint16_t* ptr = (uint16_t*)data;
    uint32_t sum = 0;
    
    while (size > 1) {
        sum += ntohs(*ptr++);
        size -= 2;
    }
    
    if (size > 0) {
        sum += *(uint8_t*)ptr << 8;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return htons(~sum);
}

void ip_print_packet(ip_packet_t* packet) {
    char src_ip[16], dst_ip[16];
    ip_to_string(packet->header.source_ip, src_ip);
    ip_to_string(packet->header.dest_ip, dst_ip);
    
    printf("IP Packet: %s -> %s [Proto: %d, TTL: %d, Size: %d]\n",
           src_ip, dst_ip, packet->header.protocol, packet->header.ttl,
           ntohs(packet->header.total_length));
}

void ip_set_source_ip(uint8_t* ip) {
    for (int i = 0; i < 4; i++) {
        source_ip[i] = ip[i];
    }
}

uint8_t* ip_get_source_ip() {
    return source_ip;
}

int ip_init() {
    if (ip_initialized) {
        return 0;
    }
    
    ip_initialized = 1;
    printf("IP protokolÃ¼ baÅŸlatÄ±ldÄ±. Kaynak IP: %d.%d.%d.%d\n", 
           source_ip[0], source_ip[1], source_ip[2], source_ip[3]);
    
    return 0;
}

int ip_route_packet(uint8_t* dest_ip, mac_addr_t* next_hop_mac) {
    // Basit routing: aynÄ± aÄŸda ise doÄŸrudan, deÄŸilse gateway
    uint8_t gateway[4] = {192, 168, 1, 1}; // VarsayÄ±lan gateway
    
    // AynÄ± aÄŸda mÄ± kontrol et (192.168.1.x)
    if (dest_ip[0] == source_ip[0] && dest_ip[1] == source_ip[1] && 
        dest_ip[2] == source_ip[2]) {
        // AynÄ± aÄŸda - doÄŸrudan ARP Ã§Ã¶zÃ¼mlemesi
        if (arp_resolve(dest_ip, next_hop_mac)) {
            return 1; // BaÅŸarÄ±lÄ±
        }
        return 0; // ARP baÅŸarÄ±sÄ±z
    } else {
        // FarklÄ± aÄŸda - gateway Ã¼zerinden
        if (arp_resolve(gateway, next_hop_mac)) {
            return 1; // BaÅŸarÄ±lÄ±
        }
        return 0; // ARP baÅŸarÄ±sÄ±z
    }
}

int ip_send_packet(uint8_t* dest_ip, uint8_t protocol, void* data, uint32_t size) {
    if (!ip_initialized || size > (IP_MAX_PACKET_SIZE - sizeof(ip_header_t))) {
        return -1;
    }
    
    mac_addr_t next_hop_mac;
    if (!ip_route_packet(dest_ip, &next_hop_mac)) {
        printf("IP: Route bulunamadÄ±\n");
        return -1;
    }
    
    ip_packet_t packet;
    
    // IP Header'Ä± oluÅŸtur
    packet.header.version_ihl = (IP_VERSION_4 << 4) | IP_HEADER_LENGTH;
    packet.header.dscp_ecn = 0;
    packet.header.total_length = htons(sizeof(ip_header_t) + size);
    packet.header.identification = htons(packet_id++);
    packet.header.flags_fragment = 0; // No fragmentation
    packet.header.ttl = IP_MAX_TTL;
    packet.header.protocol = protocol;
    packet.header.header_checksum = 0; // GeÃ§ici olarak 0
    
    // IP adreslerini kopyala
    for (int i = 0; i < 4; i++) {
        packet.header.source_ip[i] = source_ip[i];
        packet.header.dest_ip[i] = dest_ip[i];
    }
    
    // Checksum hesapla
    packet.header.header_checksum = ip_calculate_checksum(&packet.header, sizeof(ip_header_t));
    
    // Veriyi kopyala
    memcpy(packet.data, data, size);
    
    printf("IP paketi gÃ¶nderiliyor: ");
    char dst_str[16];
    ip_to_string(dest_ip, dst_str);
    printf("%s [Size: %d]\n", dst_str, sizeof(ip_header_t) + size);
    
    // Ethernet Ã¼zerinden gÃ¶nder
    return ethernet_send_frame(&next_hop_mac, ETHERNET_TYPE_IPV4, 
                              &packet, sizeof(ip_header_t) + size);
}

int ip_receive_packet(ip_packet_t* packet) {
    if (!ip_initialized) {
        return -1;
    }
    
    // Ethernet'den packet al
    ethernet_frame_t frame;
    int result = ethernet_receive_frame(&frame);
    if (result <= 0) {
        return -1; // Paket yok
    }
    
    // Ethernet tipini kontrol et
    if (ntohs(frame.header.type) != ETHERNET_TYPE_IPV4) {
        return -1; // IP paketi deÄŸil
    }
    
    // IP paketini kopyala
    memcpy(packet, frame.payload, sizeof(ip_header_t));
    
    // Header uzunluÄŸunu kontrol et
    uint8_t ihl = packet->header.version_ihl & 0x0F;
    if (ihl < 5) {
        return -1; // GeÃ§ersiz header
    }
    
    uint16_t total_length = ntohs(packet->header.total_length);
    if (total_length < sizeof(ip_header_t)) {
        return -1; // GeÃ§ersiz uzunluk
    }
    
    // Veriyi kopyala
    uint32_t data_size = total_length - sizeof(ip_header_t);
    memcpy(packet->data, frame.payload + sizeof(ip_header_t), data_size);
    
    return total_length;
}

int ip_process_packet(ip_packet_t* packet, uint32_t size) {
    if (!ip_initialized || size < sizeof(ip_header_t)) {
        return -1;
    }
    
    // Version ve IHL kontrolÃ¼
    uint8_t version = (packet->header.version_ihl >> 4) & 0x0F;
    uint8_t ihl = packet->header.version_ihl & 0x0F;
    
    if (version != IP_VERSION_4 || ihl < 5) {
        printf("IP: GeÃ§ersiz version veya IHL\n");
        return -1;
    }
    
    // Checksum kontrolÃ¼
    uint16_t received_checksum = packet->header.header_checksum;
    packet->header.header_checksum = 0;
    uint16_t calculated_checksum = ip_calculate_checksum(&packet->header, ihl * 4);
    
    if (received_checksum != calculated_checksum) {
        printf("IP: Checksum hatasÄ±\n");
        return -1;
    }
    
    // Hedef IP bizim mi?
    if (!ip_equal(packet->header.dest_ip, source_ip)) {
        // Paket bize deÄŸil - forward et (routing)
        printf("IP: Paket forward ediliyor\n");
        return 0;
    }
    
    printf("IP paketi alÄ±ndÄ±: ");
    ip_print_packet(packet);
    
    // Protokole gÃ¶re iÅŸle
    switch (packet->header.protocol) {
        case IP_PROTOCOL_ICMP:
            printf("IP: ICMP paketi\n");
            // ICMP iÅŸle
            break;
        case IP_PROTOCOL_TCP:
            printf("IP: TCP paketi\n");
            tcp_process_packet(packet, size - sizeof(ip_header_t));
            break;
        case IP_PROTOCOL_UDP:
            printf("IP: UDP paketi\n");
            udp_process_packet(packet, size - sizeof(ip_header_t));
            break;
        default:
            printf("IP: Bilinmeyen protokol: %d\n", packet->header.protocol);
            break;
    }
    
    return 0;
}
