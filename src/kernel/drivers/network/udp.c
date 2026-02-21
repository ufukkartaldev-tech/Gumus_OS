#include "udp.h"
#include "../../core/io.h"
#include "../../core/string.h"
#include "../../core/memory.h"

int udp_init() {
    printf("UDP protokolu baslatildi.\n");
    return 0;
}

uint16_t udp_calculate_checksum(udp_header_t* header, uint8_t* src_ip, uint8_t* dst_ip, void* data, uint32_t size) {
    udp_pseudo_header_t pseudo;
    memcpy(pseudo.source_ip, src_ip, 4);
    memcpy(pseudo.dest_ip, dst_ip, 4);
    pseudo.zero = 0;
    pseudo.protocol = IP_PROTOCOL_UDP;
    pseudo.udp_length = header->length;

    uint32_t sum = 0;
    uint16_t* ptr = (uint16_t*)&pseudo;
    for (int i = 0; i < sizeof(pseudo) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    ptr = (uint16_t*)header;
    for (int i = 0; i < sizeof(udp_header_t) / 2; i++) {
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

int udp_send(uint8_t* dest_ip, uint16_t src_port, uint16_t dst_port, void* data, uint32_t size) {
    uint32_t total_size = sizeof(udp_header_t) + size;
    udp_packet_t* packet = (udp_packet_t*)kmalloc(total_size);
    if (!packet) return -1;

    packet->header.source_port = htons(src_port);
    packet->header.dest_port = htons(dst_port);
    packet->header.length = htons(total_size);
    packet->header.checksum = 0;

    memcpy(packet->data, data, size);

    packet->header.checksum = udp_calculate_checksum(&packet->header, ip_get_source_ip(), dest_ip, data, size);

    printf("UDP: Paket gonderiliyor %d -> %d [Boyut: %d]\n", src_port, dst_port, total_size);
    
    int ret = ip_send_packet(dest_ip, IP_PROTOCOL_UDP, packet, total_size);
    kfree(packet);
    return ret;
}

int udp_process_packet(ip_packet_t* ip_packet, uint32_t size) {
    if (size < sizeof(udp_header_t)) return -1;

    udp_header_t* header = (udp_header_t*)ip_packet->data;
    uint16_t src_port = ntohs(header->source_port);
    uint16_t dst_port = ntohs(header->dest_port);
    uint16_t length = ntohs(header->length);

    printf("UDP: Paket alindi %d -> %d [Boyut: %d]\n", src_port, dst_port, length);

    // Veri kısmını işle (basit bir echo veya port kontrolü yapılabilir)
    void* data = ip_packet->data + sizeof(udp_header_t);
    uint32_t data_len = length - sizeof(udp_header_t);

    // Gelen veriyi (data) ekrana basabiliriz veya bir callback sistemine iletebiliriz
    // Şimdilik sadece boyutu yazdıralım.
    
    return 0;
}
