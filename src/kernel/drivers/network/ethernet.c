#include "ethernet.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "printf.h"

// Network byte order functions
static uint16_t htons(uint16_t hostshort) {
    return ((hostshort >> 8) & 0xFF) | ((hostshort & 0xFF) << 8);
}

// Global DeÄŸiÅŸkenler
static mac_addr_t our_mac = {{0x52, 0x54, 0x00, 0x12, 0x34, 0x56}}; // VarsayÄ±lan MAC
static int ethernet_initialized = 0;

// MAC Adresi Utility FonksiyonlarÄ±
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

// Ethernet SÃ¼rÃ¼cÃ¼ FonksiyonlarÄ±
int ethernet_init() {
    if (ethernet_initialized) {
        return 0; // Zaten baÅŸlatÄ±lmÄ±ÅŸ
    }
    
    // Network kartÄ± burada baÅŸlatÄ±lacak
    // Åimdilik varsayÄ±lan MAC adresini kullan
    
    ethernet_initialized = 1;
    printf("Ethernet sÃ¼rÃ¼cÃ¼sÃ¼ baÅŸlatÄ±ldÄ±. MAC: ");
    ethernet_print_mac(&our_mac);
    printf("\n");
    
    return 0;
}

int ethernet_send_frame(mac_addr_t* dest, uint16_t type, void* data, uint32_t size) {
    if (!ethernet_initialized || size > ETHERNET_MAX_PAYLOAD) {
        return -1;
    }
    
    // Frame oluÅŸtur
    ethernet_frame_t frame;
    
    // Header'Ä± doldur
    ethernet_copy_mac(&frame.header.dest, dest);
    ethernet_copy_mac(&frame.header.src, &our_mac);
    frame.header.type = htons(type);
    
    // Payload'u kopyala
    memcpy(frame.payload, data, size);
    
    // Network kartÄ±na gÃ¶nder
    // Bu kÄ±sÄ±m network kartÄ± sÃ¼rÃ¼cÃ¼sÃ¼ne baÄŸlÄ±
    // Åimdilik debug bilgisi yaz
    printf("Frame gÃ¶nderiliyor: ");
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
    
    // Network kartÄ±ndan frame al
    // Bu kÄ±sÄ±m network kartÄ± sÃ¼rÃ¼cÃ¼sÃ¼ne baÄŸlÄ±
    // Åimdilik simÃ¼le edelim
    
    return 0; // HenÃ¼z frame yok
}

void ethernet_set_mac(mac_addr_t* mac) {
    ethernet_copy_mac(&our_mac, mac);
}

mac_addr_t ethernet_get_mac() {
    return our_mac;
}

// SÃ¼rÃ¼cÃ¼ Interface FonksiyonlarÄ±
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

// Ethernet SÃ¼rÃ¼cÃ¼ YapÄ±sÄ±
driver_t ethernet_driver = {
    .name = "ethernet",
    .type = DRIVER_TYPE_NET,
    .init = ethernet_driver_init,
    .read = ethernet_driver_read,
    .write = ethernet_driver_write,
    .ioctl = ethernet_driver_ioctl,
    .shutdown = ethernet_driver_shutdown
};
