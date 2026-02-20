#include "ethernet.h"
#include "../../core/io.h"
#include "../../core/string.h"
#include "../../core/memory.h"

// Global Değişkenler
static mac_addr_t our_mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x56}}; // Varsayılan MAC
static int ethernet_initialized = 0;

// MAC Adresi Utility Fonksiyonları
int ethernet_mac_equal(mac_addr_t* a, mac_addr_t* b) {
    for (int i = 0; i < ETHERNET_ADDR_LEN; i++) {
        if (a->addr[i] != b->addr[i]) {
            return 0;
        }
    }
    return 1;
}

int ethernet_mac_is_broadcast(mac_addr_t* mac) {
    for (int i = 0; i < ETHERNET_ADDR_LEN; i++) {
        if (mac->addr[i] != 0xFF) {
            return 0;
        }
    }
    return 1;
}

int ethernet_mac_is_multicast(mac_addr_t* mac) {
    return (mac->addr[0] & 0x01) != 0;
}

void ethernet_copy_mac(mac_addr_t* dest, mac_addr_t* src) {
    for (int i = 0; i < ETHERNET_ADDR_LEN; i++) {
        dest->addr[i] = src->addr[i];
    }
}

void ethernet_print_mac(mac_addr_t* mac) {
    for (int i = 0; i < ETHERNET_ADDR_LEN; i++) {
        if (i > 0) printf(":");
        printf("%02X", mac->addr[i]);
    }
}

// Ethernet Sürücü Fonksiyonları
int ethernet_init() {
    if (ethernet_initialized) {
        return 0; // Zaten başlatılmış
    }
    
    // Network kartı burada başlatılacak
    // Şimdilik varsayılan MAC adresini kullan
    
    ethernet_initialized = 1;
    printf("Ethernet sürücüsü başlatıldı. MAC: ");
    ethernet_print_mac(&our_mac);
    printf("\n");
    
    return 0;
}

int ethernet_send_frame(mac_addr_t* dest, uint16_t type, void* data, uint32_t size) {
    if (!ethernet_initialized || size > ETHERNET_MAX_PAYLOAD) {
        return -1;
    }
    
    // Frame oluştur
    ethernet_frame_t frame;
    
    // Header'ı doldur
    ethernet_copy_mac(&frame.header.dest, dest);
    ethernet_copy_mac(&frame.header.src, &our_mac);
    frame.header.type = htons(type);
    
    // Payload'u kopyala
    memcpy(frame.payload, data, size);
    
    // Network kartına gönder
    // Bu kısım network kartı sürücüsüne bağlı
    // Şimdilik debug bilgisi yaz
    printf("Frame gönderiliyor: ");
    ethernet_print_mac(dest);
    printf(" -> ");
    ethernet_print_mac(&our_mac);
    printf(" [Type: 0x%04X, Size: %d]\n", type, size);
    
    return size;
}

int ethernet_receive_frame(ethernet_frame_t* frame) {
    if (!ethernet_initialized) {
        return -1;
    }
    
    // Network kartından frame al
    // Bu kısım network kartı sürücüsüne bağlı
    // Şimdilik simüle edelim
    
    return 0; // Henüz frame yok
}

void ethernet_set_mac(mac_addr_t* mac) {
    ethernet_copy_mac(&our_mac, mac);
}

mac_addr_t ethernet_get_mac() {
    return our_mac;
}

// Sürücü Interface Fonksiyonları
static int ethernet_driver_init(void) {
    return ethernet_init();
}

static int ethernet_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    return ethernet_receive_frame((ethernet_frame_t*)buffer);
}

static int ethernet_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    ethernet_frame_t* frame = (ethernet_frame_t*)buffer;
    return ethernet_send_frame(&frame->header.dest, frame->header.type, 
                              frame->payload, size - ETHERNET_HEADER_SIZE);
}

static int ethernet_driver_ioctl(uint32_t command, void* arg) {
    switch (command) {
        case 0: // Get MAC
            ethernet_copy_mac((mac_addr_t*)arg, &our_mac);
            return 0;
        case 1: // Set MAC
            ethernet_set_mac((mac_addr_t*)arg);
            return 0;
        default:
            return -1;
    }
}

static int ethernet_driver_shutdown(void) {
    ethernet_initialized = 0;
    return 0;
}

// Ethernet Sürücü Yapısı
driver_t ethernet_driver = {
    .name = "ethernet",
    .type = DRIVER_TYPE_NET,
    .init = ethernet_driver_init,
    .read = ethernet_driver_read,
    .write = ethernet_driver_write,
    .ioctl = ethernet_driver_ioctl,
    .shutdown = ethernet_driver_shutdown
};
