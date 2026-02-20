#include "arp.h"
#include "../../core/io.h"
#include "../../core/string.h"
#include "../../core/memory.h"

// Global Değişkenler
static uint8_t our_ip[4] = {192, 168, 1, 100}; // Varsayılan IP
static arp_cache_entry_t* arp_cache = NULL;
static int arp_initialized = 0;

// IP Adresi Utility Fonksiyonları
void ip_to_string(uint8_t* ip, char* str) {
    sprintf(str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void string_to_ip(char* str, uint8_t* ip) {
    int a, b, c, d;
    sscanf(str, "%d.%d.%d.%d", &a, &b, &c, &d);
    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;
}

int ip_equal(uint8_t* a, uint8_t* b) {
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

void arp_print_packet(arp_packet_t* packet) {
    char sender_ip[16], target_ip[16];
    ip_to_string(packet->sender_proto_addr, sender_ip);
    ip_to_string(packet->target_proto_addr, target_ip);
    
    printf("ARP Packet: %s (%02X:%02X:%02X:%02X:%02X:%02X) -> %s (%02X:%02X:%02X:%02X:%02X:%02X) [%s]\n",
           sender_ip,
           packet->sender_hw_addr.addr[0], packet->sender_hw_addr.addr[1],
           packet->sender_hw_addr.addr[2], packet->sender_hw_addr.addr[3],
           packet->sender_hw_addr.addr[4], packet->sender_hw_addr.addr[5],
           target_ip,
           packet->target_hw_addr.addr[0], packet->target_hw_addr.addr[1],
           packet->target_hw_addr.addr[2], packet->target_hw_addr.addr[3],
           packet->target_hw_addr.addr[4], packet->target_hw_addr.addr[5],
           packet->operation == ARP_OPERATION_REQUEST ? "Request" : "Reply");
}

// ARP Cache Fonksiyonları
void arp_cache_add(uint8_t* ip_addr, mac_addr_t* mac_addr) {
    // Önce bu IP için mevcut entry var mı kontrol et
    arp_cache_entry_t* entry = arp_cache;
    while (entry) {
        if (ip_equal(entry->ip_addr, ip_addr)) {
            // Mevcut entry'i güncelle
            ethernet_copy_mac(&entry->mac_addr, mac_addr);
            entry->timeout = 300; // 5 dakika
            return;
        }
        entry = entry->next;
    }
    
    // Yeni entry oluştur
    entry = (arp_cache_entry_t*)kmalloc(sizeof(arp_cache_entry_t));
    if (!entry) {
        return;
    }
    
    for (int i = 0; i < 4; i++) {
        entry->ip_addr[i] = ip_addr[i];
    }
    ethernet_copy_mac(&entry->mac_addr, mac_addr);
    entry->timeout = 300; // 5 dakika
    entry->next = arp_cache;
    arp_cache = entry;
    
    printf("ARP Cache: %s -> ", "");
    ethernet_print_mac(mac_addr);
    printf(" eklendi\n");
}

int arp_cache_lookup(uint8_t* ip_addr, mac_addr_t* mac_addr) {
    arp_cache_entry_t* entry = arp_cache;
    while (entry) {
        if (ip_equal(entry->ip_addr, ip_addr)) {
            ethernet_copy_mac(mac_addr, &entry->mac_addr);
            return 1; // Bulundu
        }
        entry = entry->next;
    }
    return 0; // Bulunamadı
}

void arp_cache_cleanup() {
    arp_cache_entry_t* current = arp_cache;
    arp_cache_entry_t* prev = NULL;
    
    while (current) {
        if (current->timeout <= 0) {
            // Süresi dolmuş entry'i sil
            if (prev) {
                prev->next = current->next;
            } else {
                arp_cache = current->next;
            }
            arp_cache_entry_t* to_delete = current;
            current = current->next;
            kfree(to_delete);
        } else {
            current->timeout--;
            prev = current;
            current = current->next;
        }
    }
}

// ARP Fonksiyonları
int arp_init() {
    if (arp_initialized) {
        return 0;
    }
    
    arp_cache = NULL;
    arp_initialized = 1;
    
    printf("ARP protokolü başlatıldı. IP: %d.%d.%d.%d\n", 
           our_ip[0], our_ip[1], our_ip[2], our_ip[3]);
    
    return 0;
}

int arp_send_request(uint8_t* target_ip) {
    if (!arp_initialized) {
        return -1;
    }
    
    arp_packet_t packet;
    
    packet.hardware_type = htons(ARP_HARDWARE_TYPE_ETHERNET);
    packet.protocol_type = htons(ARP_PROTOCOL_TYPE_IPV4);
    packet.hw_addr_len = ARP_HW_ADDR_LEN;
    packet.proto_addr_len = ARP_PROTO_ADDR_LEN;
    packet.operation = htons(ARP_OPERATION_REQUEST);
    
    ethernet_copy_mac(&packet.sender_hw_addr, ethernet_get_mac());
    for (int i = 0; i < 4; i++) {
        packet.sender_proto_addr[i] = our_ip[i];
        packet.target_proto_addr[i] = target_ip[i];
    }
    
    // Target MAC adresi broadcast (FF:FF:FF:FF:FF:FF)
    mac_addr_t broadcast = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    ethernet_copy_mac(&packet.target_hw_addr, &broadcast);
    
    printf("ARP Request gönderiliyor: ");
    char target_str[16];
    ip_to_string(target_ip, target_str);
    printf("%s\n", target_str);
    
    return ethernet_send_frame(&broadcast, ETHERNET_TYPE_ARP, &packet, sizeof(packet));
}

int arp_send_reply(uint8_t* target_ip, mac_addr_t* target_mac) {
    if (!arp_initialized) {
        return -1;
    }
    
    arp_packet_t packet;
    
    packet.hardware_type = htons(ARP_HARDWARE_TYPE_ETHERNET);
    packet.protocol_type = htons(ARP_PROTOCOL_TYPE_IPV4);
    packet.hw_addr_len = ARP_HW_ADDR_LEN;
    packet.proto_addr_len = ARP_PROTO_ADDR_LEN;
    packet.operation = htons(ARP_OPERATION_REPLY);
    
    ethernet_copy_mac(&packet.sender_hw_addr, ethernet_get_mac());
    ethernet_copy_mac(&packet.target_hw_addr, target_mac);
    
    for (int i = 0; i < 4; i++) {
        packet.sender_proto_addr[i] = our_ip[i];
        packet.target_proto_addr[i] = target_ip[i];
    }
    
    printf("ARP Reply gönderiliyor: ");
    ethernet_print_mac(target_mac);
    printf("\n");
    
    return ethernet_send_frame(target_mac, ETHERNET_TYPE_ARP, &packet, sizeof(packet));
}

int arp_process_packet(arp_packet_t* packet, mac_addr_t* sender_mac) {
    if (!arp_initialized) {
        return -1;
    }
    
    // Sadece Ethernet/IP destekleniyor
    if (ntohs(packet->hardware_type) != ARP_HARDWARE_TYPE_ETHERNET ||
        ntohs(packet->protocol_type) != ARP_PROTOCOL_TYPE_IPV4) {
        return -1;
    }
    
    // Göndereni cache'e ekle
    arp_cache_add(packet->sender_proto_addr, &packet->sender_hw_addr);
    
    uint16_t operation = ntohs(packet->operation);
    
    if (operation == ARP_OPERATION_REQUEST) {
        // Bu request bizim için mi?
        if (ip_equal(packet->target_proto_addr, our_ip)) {
            // ARP Reply gönder
            arp_send_reply(packet->sender_proto_addr, &packet->sender_hw_addr);
        }
    }
    
    return 0;
}

int arp_resolve(uint8_t* ip_addr, mac_addr_t* mac_addr) {
    // Önce cache'te ara
    if (arp_cache_lookup(ip_addr, mac_addr)) {
        return 1; // Cache'te bulundu
    }
    
    // Cache'te yoksa ARP request gönder
    arp_send_request(ip_addr);
    
    // Gerçek bir implementasyonda burada reply beklenir
    // Şimdilik başarısız döndürelim
    return 0;
}
