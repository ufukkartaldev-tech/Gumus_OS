#include "dns.h"
#include "ip.h"
#include "udp.h"
#include "string.h"
#include "memory.h"
#include "stdio.h"
#include "stdlib.h"
#include "printf.h"
#include "dhcp.h"

// Global DNS resolver
static dns_resolver_t dns_resolver;
static uint32_t dns_timer = 0;

// Rastgele transaction ID oluştur
uint16_t dns_generate_id() {
    static uint16_t counter = 0x1337;
    return counter++;
}

// Domain adını DNS formatında encode et
int dns_encode_name(const char* domain, uint8_t* buffer) {
    int len = strlen(domain);
    int label_start = 0;
    int buffer_pos = 0;
    
    for (int i = 0; i <= len; i++) {
        if (domain[i] == '.' || domain[i] == '\0') {
            int label_len = i - label_start;
            
            if (label_len > 0 && label_len <= DNS_MAX_LABELS) {
                buffer[buffer_pos++] = label_len;
                memcpy(&buffer[buffer_pos], &domain[label_start], label_len);
                buffer_pos += label_len;
            }
            
            label_start = i + 1;
        }
    }
    
    buffer[buffer_pos++] = 0; // Sonlandırıcı
    return buffer_pos;
}

// DNS formatında encode edilmiş domain adını decode et
int dns_decode_name(const uint8_t* buffer, const uint8_t* start, char* domain) {
    const uint8_t* ptr = start;
    int domain_pos = 0;
    int jumped = 0;
    const uint8_t* original_start = start;
    
    while (*ptr != 0) {
        // Compression pointer kontrolü (11 bits başta)
        if ((*ptr & 0xC0) == 0xC0) {
            if (!jumped) {
                original_start = ptr + 2;
            }
            
            uint16_t offset = ((*ptr & 0x3F) << 8) | *(ptr + 1);
            ptr = buffer + offset;
            jumped = 1;
        } else {
            int label_len = *ptr;
            ptr++;
            
            if (domain_pos > 0) {
                domain[domain_pos++] = '.';
            }
            
            for (int i = 0; i < label_len; i++) {
                domain[domain_pos++] = ptr[i];
            }
            
            ptr += label_len;
        }
    }
    
    domain[domain_pos] = '\0';
    
    if (jumped) {
        return (original_start - start) + 2;
    } else {
        return (ptr - start) + 1;
    }
}

// DNS cache'e ekle
int dns_add_cache_entry(const char* domain, uint8_t* ip, uint32_t ttl) {
    // Önce mevcut entry'yi kontrol et
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_resolver.cache[i].valid && 
            strcmp(dns_resolver.cache[i].domain, domain) == 0) {
            // Mevcut entry'yi güncelle
            memcpy(dns_resolver.cache[i].ip, ip, 4);
            dns_resolver.cache[i].ttl = ttl;
            dns_resolver.cache[i].timestamp = dns_timer;
            return i;
        }
    }
    
    // Yeni slot bul
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (!dns_resolver.cache[i].valid) {
            strcpy(dns_resolver.cache[i].domain, domain);
            memcpy(dns_resolver.cache[i].ip, ip, 4);
            dns_resolver.cache[i].ttl = ttl;
            dns_resolver.cache[i].timestamp = dns_timer;
            dns_resolver.cache[i].valid = 1;
            return i;
        }
    }
    
    // En eski entry'yi değiştir
    uint32_t oldest_time = dns_timer;
    int oldest_index = 0;
    
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_resolver.cache[i].timestamp < oldest_time) {
            oldest_time = dns_resolver.cache[i].timestamp;
            oldest_index = i;
        }
    }
    
    strcpy(dns_resolver.cache[oldest_index].domain, domain);
    memcpy(dns_resolver.cache[oldest_index].ip, ip, 4);
    dns_resolver.cache[oldest_index].ttl = ttl;
    dns_resolver.cache[oldest_index].timestamp = dns_timer;
    dns_resolver.cache[oldest_index].valid = 1;
    
    return oldest_index;
}

// DNS cache'te ara
int dns_lookup_cache(const char* domain, uint8_t* ip) {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_resolver.cache[i].valid && 
            strcmp(dns_resolver.cache[i].domain, domain) == 0) {
            
            // TTL kontrolü
            uint32_t elapsed = dns_timer - dns_resolver.cache[i].timestamp;
            if (elapsed < dns_resolver.cache[i].ttl) {
                memcpy(ip, dns_resolver.cache[i].ip, 4);
                return 0;
            } else {
                // Süresi dolmuş, geçersiz yap
                dns_resolver.cache[i].valid = 0;
            }
        }
    }
    
    return -1;
}

// DNS cache temizliği
void dns_cache_cleanup() {
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_resolver.cache[i].valid) {
            uint32_t elapsed = dns_timer - dns_resolver.cache[i].timestamp;
            if (elapsed >= dns_resolver.cache[i].ttl) {
                dns_resolver.cache[i].valid = 0;
            }
        }
    }
}

// DNS sorgusu gönder
int dns_send_query(const char* domain, uint16_t type) {
    uint8_t packet[DNS_MAX_PACKET_SIZE];
    uint32_t packet_size = 0;
    
    // Header
    dns_header_t* header = (dns_header_t*)packet;
    header->id = htons(dns_resolver.transaction_id);
    header->flags = htons(0x0100); // Standard query
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
    
    packet_size = sizeof(dns_header_t);
    
    // Question
    int name_len = dns_encode_name(domain, &packet[packet_size]);
    packet_size += name_len;
    
    uint16_t* qtype = (uint16_t*)&packet[packet_size];
    *qtype = htons(type);
    packet_size += 2;
    
    uint16_t* qclass = (uint16_t*)&packet[packet_size];
    *qclass = htons(DNS_CLASS_IN);
    packet_size += 2;
    
    // DNS server'a gönder
    return udp_send(dns_resolver.dns_server, 12345, DNS_PORT, packet, packet_size);
}

// DNS record parse et
int dns_parse_record(const uint8_t* buffer, uint32_t* offset, dns_record_t* record) {
    // Name (compression destekli)
    int name_len = dns_decode_name(buffer, buffer + *offset, (char*)record->name);
    *offset += name_len;
    
    // Type, Class, TTL, RDLength
    record->type = ntohs(*(uint16_t*)(buffer + *offset));
    *offset += 2;
    
    record->class = ntohs(*(uint16_t*)(buffer + *offset));
    *offset += 2;
    
    record->ttl = ntohl(*(uint32_t*)(buffer + *offset));
    *offset += 4;
    
    record->rdlength = ntohs(*(uint16_t*)(buffer + *offset));
    *offset += 2;
    
    // Data
    if (record->rdlength <= sizeof(record->rdata)) {
        memcpy(record->rdata, buffer + *offset, record->rdlength);
        *offset += record->rdlength;
    } else {
        // Data çok uzun, atla
        *offset += record->rdlength;
        return -1;
    }
    
    return 0;
}

// DNS cevabını işle
int dns_process_response(dns_packet_t* packet, uint32_t size) {
    // Header kontrolü
    if (ntohs(packet->header.id) != dns_resolver.transaction_id) {
        return -1;
    }
    
    uint16_t flags = ntohs(packet->header.flags);
    uint8_t rcode = flags & 0x0F;
    
    if (rcode != DNS_RESPONSE_NO_ERROR) {
        printf("DNS Hata: %d\n", rcode);
        return -1;
    }
    
    uint32_t offset = sizeof(dns_header_t);
    
    // Questions'ları atla
    uint16_t qdcount = ntohs(packet->header.qdcount);
    for (int i = 0; i < qdcount; i++) {
        // Name
        char name[DNS_MAX_NAME_LENGTH];
        int name_len = dns_decode_name((uint8_t*)packet, (uint8_t*)packet + offset, name);
        offset += name_len;
        
        // Type ve Class
        offset += 4; // qtype + qclass
    }
    
    // Answer'ları işle
    uint16_t ancount = ntohs(packet->header.ancount);
    for (int i = 0; i < ancount; i++) {
        dns_record_t record;
        if (dns_parse_record((uint8_t*)packet, &offset, &record) == 0) {
            if (record.type == DNS_TYPE_A && record.rdlength == 4) {
                // A record bulundu, cache'e ekle
                char domain[DNS_MAX_NAME_LENGTH];
                dns_decode_name((uint8_t*)packet, (uint8_t*)packet + sizeof(dns_header_t), domain);
                
                dns_add_cache_entry(domain, record.rdata, record.ttl);
                printf("DNS Cache: %s -> %d.%d.%d.%d\n", domain,
                       record.rdata[0], record.rdata[1], record.rdata[2], record.rdata[3]);
            }
        }
    }
    
    return 0;
}

// Domain adını resolve et
int dns_resolve(const char* domain, uint8_t* ip) {
    if (!dns_is_valid_domain(domain)) {
        printf("Geçersiz domain adı: %s\n", domain);
        return -1;
    }
    
    // Önce cache'te ara
    if (dns_lookup_cache(domain, ip) == 0) {
        printf("DNS Cache hit: %s\n", domain);
        return 0;
    }
    
    printf("DNS Query: %s\n", domain);
    
    // DNS sorgusu gönder
    dns_resolver.transaction_id = dns_generate_id();
    if (dns_send_query(domain, DNS_TYPE_A) < 0) {
        printf("DNS sorgusu gönderilemedi.\n");
        return -1;
    }
    
    // Cevabı bekle (basit implementasyon)
    // Gerçek implementasyon'da callback veya polling kullanılmalı
    dns_resolver.timeout = dns_timer + 5; // 5 saniye timeout
    
    return -2; // Cevap bekleniyor
}

// DNS query gönder ve cevabı al
int dns_query(const char* domain, uint16_t type, dns_packet_t* response) {
    dns_resolver.transaction_id = dns_generate_id();
    
    if (dns_send_query(domain, type) < 0) {
        return -1;
    }
    
    // Cevabı bekle
    dns_resolver.timeout = dns_timer + 5;
    
    return 0;
}

// DNS server'ı ayarla
int dns_set_server(uint8_t* server_ip) {
    memcpy(dns_resolver.dns_server, server_ip, 4);
    printf("DNS Server: %d.%d.%d.%d\n", 
           server_ip[0], server_ip[1], server_ip[2], server_ip[3]);
    return 0;
}

// Domain adı geçerliliğini kontrol et
int dns_is_valid_domain(const char* domain) {
    if (!domain || strlen(domain) == 0 || strlen(domain) >= DNS_MAX_NAME_LENGTH) {
        return 0;
    }
    
    int len = strlen(domain);
    int label_len = 0;
    
    for (int i = 0; i < len; i++) {
        if (domain[i] == '.') {
            if (label_len == 0 || label_len > DNS_MAX_LABELS) {
                return 0;
            }
            label_len = 0;
        } else if ((domain[i] >= 'a' && domain[i] <= 'z') ||
                   (domain[i] >= 'A' && domain[i] <= 'Z') ||
                   (domain[i] >= '0' && domain[i] <= '9') ||
                   domain[i] == '-') {
            label_len++;
        } else {
            return 0;
        }
    }
    
    return (label_len > 0 && label_len <= DNS_MAX_LABELS);
}

// IP adresini string'e çevir
void dns_ip_to_string(uint8_t* ip, char* str) {
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

// String'i IP adresine çevir
int dns_string_to_ip(const char* str, uint8_t* ip) {
    int a, b, c, d;
    if (sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d) != 4) {
        return -1;
    }
    
    if (a < 0 || a > 255 || b < 0 || b > 255 || c < 0 || c > 255 || d < 0 || d > 255) {
        return -1;
    }
    
    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;
    
    return 0;
}

// DNS cache'i yazdır
void dns_print_cache() {
    printf("=== DNS Cache ===\n");
    int count = 0;
    
    for (int i = 0; i < DNS_CACHE_SIZE; i++) {
        if (dns_resolver.cache[i].valid) {
            uint32_t elapsed = dns_timer - dns_resolver.cache[i].timestamp;
            uint32_t remaining = dns_resolver.cache[i].ttl - elapsed;
            
            char ip_str[16];
            dns_ip_to_string(dns_resolver.cache[i].ip, ip_str);
            
            printf("%s -> %s (TTL: %ds)\n", 
                   dns_resolver.cache[i].domain, ip_str, remaining);
            count++;
        }
    }
    
    if (count == 0) {
        printf("Cache boş.\n");
    }
    printf("================\n");
}

// DNS'yi başlat
int dns_init() {
    printf("DNS Resolver başlatılıyor...\n");
    
    // DNS resolver yapısını sıfırla
    memset(&dns_resolver, 0, sizeof(dns_resolver_t));
    
    // Varsayılan DNS server (Google DNS)
    uint8_t default_dns[4] = {8, 8, 8, 8};
    dns_set_server(default_dns);
    
    printf("DNS Resolver hazır.\n");
    return 0;
}

// DNS timer tick (her saniye çağrılmalı)
void dns_tick() {
    dns_timer++;
    
    // Cache temizliği
    if (dns_timer % 60 == 0) { // Her dakika
        dns_cache_cleanup();
    }
}
