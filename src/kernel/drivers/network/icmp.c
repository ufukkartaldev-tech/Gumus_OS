#include "icmp.h"
#include "../../core/io.h"
#include "../../core/string.h"
#include "../../core/memory.h"

// Global Değişkenler
static int icmp_initialized = 0;
static uint16_t ping_sequence = 1;
static uint32_t ping_sent = 0;
static uint32_t ping_received = 0;

// Checksum Hesaplama
uint16_t icmp_calculate_checksum(void* data, uint32_t size) {
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

void icmp_print_packet(icmp_header_t* packet, uint32_t size) {
    switch (packet->type) {
        case ICMP_TYPE_ECHO_REQUEST:
            printf("ICMP Echo Request");
            break;
        case ICMP_TYPE_ECHO_REPLY:
            printf("ICMP Echo Reply");
            break;
        case ICMP_TYPE_DEST_UNREACHABLE:
            printf("ICMP Destination Unreachable (Code: %d)", packet->code);
            break;
        case ICMP_TYPE_TIME_EXCEEDED:
            printf("ICMP Time Exceeded");
            break;
        default:
            printf("ICMP Type: %d, Code: %d", packet->type, packet->code);
            break;
    }
    printf(" [Size: %d, Checksum: 0x%04X]\n", size, ntohs(packet->checksum));
}

int icmp_init() {
    if (icmp_initialized) {
        return 0;
    }
    
    ping_sequence = 1;
    ping_sent = 0;
    ping_received = 0;
    icmp_initialized = 1;
    
    printf("ICMP protokolü başlatıldı\n");
    return 0;
}

int icmp_send_echo(uint8_t* dest_ip, uint16_t identifier, uint16_t sequence) {
    if (!icmp_initialized) {
        return -1;
    }
    
    icmp_echo_t echo_packet;
    
    // ICMP Header'ı oluştur
    echo_packet.header.type = ICMP_TYPE_ECHO_REQUEST;
    echo_packet.header.code = 0;
    echo_packet.header.checksum = 0;
    echo_packet.header.rest_of_header = 0;
    
    echo_packet.identifier = htons(identifier);
    echo_packet.sequence_number = htons(sequence);
    
    // Test verisi doldur
    for (int i = 0; i < 64; i++) {
        echo_packet.data[i] = i;
    }
    
    // Checksum hesapla
    echo_packet.header.checksum = icmp_calculate_checksum(&echo_packet, sizeof(echo_packet));
    
    printf("ICMP Echo gönderiliyor: ");
    char dst_str[16];
    ip_to_string(dest_ip, dst_str);
    printf("%s [ID: %d, Seq: %d]\n", dst_str, identifier, sequence);
    
    ping_sent++;
    
    // IP üzerinden gönder
    return ip_send_packet(dest_ip, IP_PROTOCOL_ICMP, &echo_packet, sizeof(echo_packet));
}

int icmp_send_echo_reply(uint8_t* dest_ip, uint16_t identifier, uint16_t sequence) {
    if (!icmp_initialized) {
        return -1;
    }
    
    icmp_echo_t echo_packet;
    
    // ICMP Header'ı oluştur
    echo_packet.header.type = ICMP_TYPE_ECHO_REPLY;
    echo_packet.header.code = 0;
    echo_packet.header.checksum = 0;
    echo_packet.header.rest_of_header = 0;
    
    echo_packet.identifier = htons(identifier);
    echo_packet.sequence_number = htons(sequence);
    
    // Test verisi doldur
    for (int i = 0; i < 64; i++) {
        echo_packet.data[i] = i;
    }
    
    // Checksum hesapla
    echo_packet.header.checksum = icmp_calculate_checksum(&echo_packet, sizeof(echo_packet));
    
    printf("ICMP Echo Reply gönderiliyor: ");
    char dst_str[16];
    ip_to_string(dest_ip, dst_str);
    printf("%s [ID: %d, Seq: %d]\n", dst_str, identifier, sequence);
    
    // IP üzerinden gönder
    return ip_send_packet(dest_ip, IP_PROTOCOL_ICMP, &echo_packet, sizeof(echo_packet));
}

int icmp_process_packet(icmp_header_t* packet, uint32_t size, uint8_t* source_ip) {
    if (!icmp_initialized || size < sizeof(icmp_header_t)) {
        return -1;
    }
    
    // Checksum kontrolü
    uint16_t received_checksum = packet->checksum;
    packet->checksum = 0;
    uint16_t calculated_checksum = icmp_calculate_checksum(packet, size);
    
    if (received_checksum != calculated_checksum) {
        printf("ICMP: Checksum hatası\n");
        return -1;
    }
    
    printf("ICMP paketi alındı: ");
    icmp_print_packet(packet, size);
    
    switch (packet->type) {
        case ICMP_TYPE_ECHO_REQUEST: {
            // Echo Request'e Echo Reply gönder
            icmp_echo_t* echo = (icmp_echo_t*)packet;
            uint16_t identifier = ntohs(echo->identifier);
            uint16_t sequence = ntohs(echo->sequence_number);
            
            icmp_send_echo_reply(source_ip, identifier, sequence);
            break;
        }
        case ICMP_TYPE_ECHO_REPLY: {
            // Echo Reply alındı
            ping_received++;
            icmp_echo_t* echo = (icmp_echo_t*)packet;
            uint16_t identifier = ntohs(echo->identifier);
            uint16_t sequence = ntohs(echo->sequence_number);
            
            char src_str[16];
            ip_to_string(source_ip, src_str);
            printf("Ping reply alındı: %s [ID: %d, Seq: %d]\n", src_str, identifier, sequence);
            break;
        }
        case ICMP_TYPE_DEST_UNREACHABLE:
            printf("Hedef ulaşılamaz: Kod %d\n", packet->code);
            break;
        case ICMP_TYPE_TIME_EXCEEDED:
            printf("Zaman aşımı\n");
            break;
        default:
            printf("Bilinmeyen ICMP tipi: %d\n", packet->type);
            break;
    }
    
    return 0;
}

int ping(uint8_t* dest_ip, uint32_t count) {
    if (!icmp_initialized) {
        return -1;
    }
    
    printf("PING başlatılıyor: ");
    char dst_str[16];
    ip_to_string(dest_ip, dst_str);
    printf("%s (%d paket)\n", dst_str, count);
    
    ping_sent = 0;
    ping_received = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        icmp_send_echo(dest_ip, 0x1234, ping_sequence++);
        
        // Gerçek bir implementasyonda burada reply beklenir
        // Şimdilik simüle edelim
        // Gerçek uygulamada timer ve interrupt gerekir
        
        // Basit bekleme (simülasyon için)
        for (volatile int j = 0; j < 1000000; j++);
    }
    
    printf("Ping istatistikleri: %d gönderildi, %d alındı\n", ping_sent, ping_received);
    
    return 0;
}

void ping_start(uint8_t* dest_ip) {
    printf("Sürekli ping başlatılıyor: ");
    char dst_str[16];
    ip_to_string(dest_ip, dst_str);
    printf("%s\n", dst_str);
    
    // Sürekli ping için thread başlatılabilir
    // Şimdilik tek seferlik ping yap
    ping(dest_ip, 4);
}

void ping_stop() {
    printf("Ping durduruldu\n");
    // Ping thread'ini durdur
}
