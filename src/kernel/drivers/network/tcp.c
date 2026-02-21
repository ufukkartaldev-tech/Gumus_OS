#include "tcp.h"
#include "../../core/io.h"
#include "../../core/string.h"
#include "../../core/memory.h"

int tcp_init() {
    printf("TCP protokolu baslatildi.\n");
    return 0;
}

uint16_t tcp_calculate_checksum(tcp_header_t* header, uint8_t* src_ip, uint8_t* dst_ip, void* data, uint32_t size) {
    // TCP de UDP gibi pseudo-header kullanır
    udp_pseudo_header_t pseudo; // udp_pseudo_header_t is generic enough for IP proto
    memcpy(pseudo.source_ip, src_ip, 4);
    memcpy(pseudo.dest_ip, dst_ip, 4);
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTOCOL_TCP;
    pseudo.udp_length = htons(sizeof(tcp_header_t) + size);

    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)&pseudo;
    for (int i = 0; i < sizeof(pseudo) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    ptr = (uint16_t*)header;
    for (int i = 0; i < sizeof(tcp_header_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    ptr = (uint16_t*)data;
    uint32_t len = size;
    while (len > 1) {
        sum += ntohs(*ptr++);
        len -= 2;
    }
    if (len > 0) {
        sum += (*(uint8_t*)ptr) << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons(~sum);
}

int tcp_send(tcp_connection_t* conn, void* data, uint32_t size, uint16_t flags) {
    uint32_t total_size = sizeof(tcp_header_t) + size;
    // Buffer allocate (kmalloc)
    uint8_t* buffer = (uint8_t*)kmalloc(total_size);
    if (!buffer) return -1;

    tcp_header_t* header = (tcp_header_t*)buffer;
    header->source_port = htons(conn->local_port);
    header->dest_port = htons(conn->remote_port);
    header->sequence_number = htonl(conn->seq);
    header->ack_number = htonl(conn->ack);
    header->flags = htons((5 << 12) | flags); // Header length is 5 words (20 bytes)
    header->window_size = htons(8192);
    header->checksum = 0;
    header->urgent_pointer = 0;

    if (size > 0 && data) {
        memcpy(buffer + sizeof(tcp_header_t), data, size);
    }

    header->checksum = tcp_calculate_checksum(header, ip_get_source_ip(), conn->remote_ip, data, size);

    printf("TCP: Paket gonderiliyor %d -> %d [Flags: 0x%X]\n", conn->local_port, conn->remote_port, flags);
    
    int ret = ip_send_packet(conn->remote_ip, IP_PROTOCOL_TCP, buffer, total_size);
    kfree(buffer);
    return ret;
}

int tcp_process_packet(ip_packet_t* ip_packet, uint32_t size) {
    if (size < sizeof(tcp_header_t)) return -1;

    tcp_header_t* header = (tcp_header_t*)ip_packet->data;
    uint16_t src_port = ntohs(header->source_port);
    uint16_t dst_port = ntohs(header->dest_port);
    uint16_t flags = ntohs(header->flags) & 0x1FF;

    printf("TCP: Paket alindi %d -> %d [Flags: 0x%X]\n", src_port, dst_port, flags);

    // Basit bir SYN -> SYN-ACK yanıtı (simülasyon)
    if (flags & TCP_FLAG_SYN) {
        printf("TCP: SYN istegi alindi, SYN-ACK gonderiliyor...\n");
        // Burada gerçek bir bağlantı yönetimi gerekir.
    }

    return 0;
}
