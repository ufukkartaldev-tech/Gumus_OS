#include "network_driver.h"
#include "memory.h"
#include "io.h"
#include "string.h"
#include "printf.h"
#include "stdio.h"
#include "stdlib.h"

// Network byte order functions
static uint16_t htons(uint16_t hostshort) {
    return ((hostshort >> 8) & 0xFF) | ((hostshort & 0xFF) << 8);
}

static uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

// Memory comparison function
static int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

static rtl8139_driver_t rtl8139_driver;
static e1000_driver_t e1000_driver;
static int rtl8139_initialized = 0;
static int e1000_initialized = 0;

// RTL8139 Registers
#define RTL8139_REG_ID           0x00
#define RTL8139_REG_MAC0_3       0x00
#define RTL8139_REG_MAC4_5       0x04
#define RTL8139_REG_MAR0         0x08
#define RTL8139_REG_TX_ADDR0     0x20
#define RTL8139_REG_TX_ADDR1     0x24
#define RTL8139_REG_TX_ADDR2     0x28
#define RTL8139_REG_TX_ADDR3     0x2C
#define RTL8139_REG_CMD          0x37
#define RTL8139_REG_CAPR         0x38
#define RTL8139_REG_CBR          0x3A
#define RTL8139_REG_IMR          0x3C
#define RTL8139_REG_ISR          0x3E
#define RTL8139_REG_TX_CONFIG    0x40
#define RTL8139_REG_RX_CONFIG    0x44
#define RTL8139_REG_TIMER_INT    0x54
#define RTL8139_REG_MPC          0x5C
#define RTL8139_REG_9346CR       0x50
#define RTL8139_REG_CONFIG0      0x51
#define RTL8139_REG_CONFIG1      0x52
#define RTL8139_REG_MSR          0x58
#define RTL8139_REG_CONFIG3      0x59
#define RTL8139_REG_CONFIG4      0x5A
#define RTL8139_REG_CONFIG5      0x5B
#define RTL8139_REG_MULTI_INTR   0x5C
#define RTL8139_REG_PHY_RESET    0x60
#define RTL8139_REG_PHY_STATUS   0x6C
#define RTL8139_REG_PHY_ANAR     0x70
#define RTL8139_REG_PHY_ANLPAR   0x74
#define RTL8139_REG_PHY_ANER     0x78
#define RTL8139_REG_RBSTART      0x30

// RTL8139 Command Bits
#define RTL8139_CMD_TX_ENABLE    0x04
#define RTL8139_CMD_RX_ENABLE    0x08
#define RTL8139_CMD_RESET        0x10

// RTL8139 Interrupt Bits
#define RTL8139_ISR_RX_OK        0x01
#define RTL8139_ISR_RX_ERR       0x02
#define RTL8139_ISR_TX_OK        0x04
#define RTL8139_ISR_TX_ERR       0x08
#define RTL8139_ISR_RX_OVERFLOW   0x10
#define RTL8139_ISR_LINK_CHANGE  0x20

// E1000 Registers
#define E1000_REG_CTRL          0x00000
#define E1000_REG_STATUS        0x00008
#define E1000_REG_EECD          0x00010
#define E1000_REG_EERD          0x00014
#define E1000_REG_CTRL_EXT      0x00018
#define E1000_REG_ICR           0x000C0
#define E1000_REG_IMS           0x000D0
#define E1000_REG_IMC           0x000D8
#define E1000_REG_RCTL          0x00100
#define E1000_REG_TCTL          0x00400
#define E1000_REG_RDBAL         0x02800
#define E1000_REG_RDBAH         0x02804
#define E1000_REG_RDLEN         0x02808
#define E1000_REG_RDH           0x02810
#define E1000_REG_RDT           0x02818
#define E1000_REG_TDBAL         0x03800
#define E1000_REG_TDBAH         0x03804
#define E1000_REG_TDLEN         0x03808
#define E1000_REG_TDH           0x03810
#define E1000_REG_TDT           0x03818
#define E1000_REG_RAL           0x05400
#define E1000_REG_RAH           0x05404

// Network Driver Functions
static int network_driver_init(void* driver) {
    network_driver_t* net_driver = (network_driver_t*)driver;
    printf("Network sÃ¼rÃ¼cÃ¼sÃ¼ baÅŸlatÄ±lÄ±yor...\n");
    return network_init(net_driver);
}

static int network_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    network_driver_t* driver = (network_driver_t*)buffer;
    if (!driver || !driver->initialized) return -1;
    
    return network_receive_packet(driver, (uint8_t*)buffer, &size);
}

static int network_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    network_driver_t* driver = (network_driver_t*)buffer;
    if (!driver || !driver->initialized) return -1;
    
    return network_send_packet(driver, (uint8_t*)buffer, size);
}

static int network_driver_ioctl(uint32_t command, void* arg) {
    network_driver_t* driver = (network_driver_t*)arg;
    if (!driver || !driver->initialized) return -1;
    
    switch (command) {
        case 0x2001: // Set MAC address
            return network_set_mac_address(driver, (uint8_t*)arg);
        case 0x2002: // Set IP config
            {
                struct ip_config {
                    uint32_t ip, subnet, gateway;
                } *config = (struct ip_config*)arg;
                return network_set_ip_config(driver, config->ip, config->subnet, config->gateway);
            }
        case 0x2003: // Send ARP request
            return network_send_arp_request(driver, *(uint32_t*)arg);
        case 0x2004: // Send ping
            return network_send_ping(driver, *(uint32_t*)arg);
    }
    return -1;
}

static int network_driver_shutdown(void* driver) {
    network_driver_t* net_driver = (network_driver_t*)driver;
    printf("Network sÃ¼rÃ¼cÃ¼sÃ¼ kapatÄ±lÄ±yor...\n");
    net_driver->initialized = 0;
    return 0;
}

// RTL8139 Driver Functions
static int rtl8139_driver_init(void) {
    printf("RTL8139 sÃ¼rÃ¼cÃ¼sÃ¼ baÅŸlatÄ±lÄ±yor...\n");
    return rtl8139_init(&rtl8139_driver.base);
}

static int rtl8139_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    if (!rtl8139_initialized) return -1;
    return rtl8139_receive(&rtl8139_driver.base, (uint8_t*)buffer, &size);
}

static int rtl8139_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    if (!rtl8139_initialized) return -1;
    return rtl8139_send(&rtl8139_driver.base, (uint8_t*)buffer, size);
}

static int rtl8139_driver_ioctl(uint32_t command, void* arg) {
    if (!rtl8139_initialized) return -1;
    return network_driver_ioctl(command, &rtl8139_driver.base);
}

static int rtl8139_driver_shutdown(void) {
    printf("RTL8139 sÃ¼rÃ¼cÃ¼sÃ¼ kapatÄ±lÄ±yor...\n");
    rtl8139_initialized = 0;
    return 0;
}

// E1000 Driver Functions
static int e1000_driver_init(void) {
    printf("Intel E1000 sÃ¼rÃ¼cÃ¼sÃ¼ baÅŸlatÄ±lÄ±yor...\n");
    return e1000_init(&e1000_driver.base);
}

static int e1000_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    if (!e1000_initialized) return -1;
    return e1000_receive(&e1000_driver.base, (uint8_t*)buffer, &size);
}

static int e1000_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    if (!e1000_initialized) return -1;
    return e1000_send(&e1000_driver.base, (uint8_t*)buffer, size);
}

static int e1000_driver_ioctl(uint32_t command, void* arg) {
    if (!e1000_initialized) return -1;
    return network_driver_ioctl(command, &e1000_driver.base);
}

static int e1000_driver_shutdown(void) {
    printf("Intel E1000 sÃ¼rÃ¼cÃ¼sÃ¼ kapatÄ±lÄ±yor...\n");
    e1000_initialized = 0;
    return 0;
}

// Network Core Functions
int network_init(network_driver_t* driver) {
    if (!driver) return -1;
    
    printf("Network aygÄ±tÄ± baÅŸlatÄ±lÄ±yor...\n");
    
    // Buffer'larÄ± baÅŸlat
    driver->rx_head = 0;
    driver->rx_tail = 0;
    driver->tx_head = 0;
    driver->tx_tail = 0;
    
    // Statistics'Ä± sÄ±fÄ±rla
    memset(&driver->stats, 0, sizeof(network_stats_t));
    
    // VarsayÄ±lan konfigÃ¼rasyon
    memset(&driver->config, 0, sizeof(network_config_t));
    
    driver->initialized = 1;
    return 0;
}

int network_send_packet(network_driver_t* driver, uint8_t* data, uint32_t size) {
    if (!driver || !driver->initialized || !data || size > NETWORK_MAX_PACKET_SIZE) {
        return -1;
    }
    
    // Hardware-specific send fonksiyonunu Ã§aÄŸÄ±r
    // Bu fonksiyon hardware driver tarafÄ±ndan override edilir
    driver->stats.packets_sent++;
    driver->stats.bytes_sent += size;
    
    return 0;
}

int network_receive_packet(network_driver_t* driver, uint8_t* buffer, uint32_t* size) {
    if (!driver || !driver->initialized || !buffer || !size) {
        return -1;
    }
    
    // RX buffer'Ä±ndan paket al
    if (driver->rx_head == driver->rx_tail) {
        return -1; // No packets available
    }
    
    network_packet_t* packet = &driver->rx_buffer[driver->rx_head];
    if (*size < packet->size) {
        return -1; // Buffer too small
    }
    
    memcpy(buffer, packet->data, packet->size);
    *size = packet->size;
    
    driver->rx_head = (driver->rx_head + 1) % 32;
    
    driver->stats.packets_received++;
    driver->stats.bytes_received += packet->size;
    
    return 0;
}

int network_set_mac_address(network_driver_t* driver, uint8_t* mac) {
    if (!driver || !driver->initialized || !mac) {
        return -1;
    }
    
    memcpy(driver->mac_address, mac, 6);
    memcpy(driver->config.mac, mac, 6);
    
    char mac_str[18];
    network_format_mac(mac, mac_str);
    printf("MAC adresi ayarlandÄ±: %s\n", mac_str);
    
    return 0;
}

int network_set_ip_config(network_driver_t* driver, uint32_t ip, uint32_t subnet_mask, uint32_t gateway) {
    if (!driver || !driver->initialized) {
        return -1;
    }
    
    driver->config.ip = ip;
    driver->config.subnet_mask = subnet_mask;
    driver->config.gateway = gateway;
    
    char ip_str[16], subnet_str[16], gateway_str[16];
    network_format_ip(ip, ip_str);
    network_format_ip(subnet_mask, subnet_str);
    network_format_ip(gateway, gateway_str);
    
    printf("IP konfigÃ¼rasyonu: IP=%s, Subnet=%s, Gateway=%s\n", ip_str, subnet_str, gateway_str);
    
    return 0;
}

int network_send_arp_request(network_driver_t* driver, uint32_t target_ip) {
    if (!driver || !driver->initialized) {
        return -1;
    }
    
    char target_ip_str[16];
    network_format_ip(target_ip, target_ip_str);
    printf("ARP request gÃ¶nderiliyor: %s\n", target_ip_str);
    
    // ARP packet oluÅŸtur
    uint8_t packet[NETWORK_ARP_SIZE];
    eth_header_t* eth = (eth_header_t*)packet;
    arp_packet_t* arp = (arp_packet_t*)(packet + sizeof(eth_header_t));
    
    // Ethernet header
    memcpy(eth->dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6); // Broadcast
    memcpy(eth->src_mac, driver->mac_address, 6);
    eth->ethertype = htons(ETH_TYPE_ARP);
    
    // ARP packet
    arp->hardware_type = htons(1); // Ethernet
    arp->protocol_type = htons(ETH_TYPE_IPv4);
    arp->hardware_len = 6;
    arp->protocol_len = 4;
    arp->operation = htons(ARP_OP_REQUEST);
    memcpy(arp->sender_mac, driver->mac_address, 6);
    arp->sender_ip = driver->config.ip;
    memset(arp->target_mac, 0, 6);
    arp->target_ip = target_ip;
    
    return network_send_packet(driver, packet, sizeof(packet));
}

int network_send_ping(network_driver_t* driver, uint32_t target_ip) {
    if (!driver || !driver->initialized) {
        return -1;
    }
    
    char target_ip_str[16];
    network_format_ip(target_ip, target_ip_str);
    printf("Ping gÃ¶nderiliyor: %s\n", target_ip_str);
    
    // ICMP echo request oluÅŸtur
    uint8_t packet[NETWORK_ICMP_SIZE];
    eth_header_t* eth = (eth_header_t*)packet;
    ip_header_t* ip = (ip_header_t*)(packet + sizeof(eth_header_t));
    icmp_header_t* icmp = (icmp_header_t*)(packet + sizeof(eth_header_t) + sizeof(ip_header_t));
    
    // Ethernet header
    // Target MAC address'i ARP table'dan almalÄ±yÄ±z (ÅŸimdilik broadcast)
    memcpy(eth->dest_mac, "\xFF\xFF\xFF\xFF\xFF\xFF", 6);
    memcpy(eth->src_mac, driver->mac_address, 6);
    eth->ethertype = htons(ETH_TYPE_IPv4);
    
    // IP header
    ip->version_ihl = 0x45; // IPv4, 5 words
    ip->tos = 0;
    ip->total_length = htons(sizeof(ip_header_t) + sizeof(icmp_header_t));
    ip->identification = htons(0x1234);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->header_checksum = 0;
    ip->src_ip = driver->config.ip;
    ip->dest_ip = target_ip;
    
    // IP checksum hesapla
    ip->header_checksum = network_checksum((uint8_t*)ip, sizeof(ip_header_t));
    
    // ICMP header
    icmp->type = ICMP_TYPE_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = htons(0x1234);
    icmp->sequence = htons(1);
    
    // ICMP checksum hesapla
    icmp->checksum = network_checksum((uint8_t*)icmp, sizeof(icmp_header_t));
    
    return network_send_packet(driver, packet, sizeof(packet));
}

uint16_t network_checksum(uint8_t* data, uint32_t size) {
    uint32_t sum = 0;
    
    // 16-bit word'larÄ± topla
    for (uint32_t i = 0; i < size; i += 2) {
        uint16_t word = (data[i] << 8) | (i + 1 < size ? data[i + 1] : 0);
        sum += word;
        
        // Overflow'Ä± handle et
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    
    // One's complement
    return ~sum & 0xFFFF;
}

void network_format_mac(uint8_t* mac, char* buffer) {
    sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void network_format_ip(uint32_t ip, char* buffer) {
    sprintf(buffer, "%d.%d.%d.%d", 
            (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

uint32_t network_parse_ip(const char* ip_string) {
    uint32_t ip = 0;
    int octets[4];
    
    if (sscanf(ip_string, "%d.%d.%d.%d", &octets[0], &octets[1], &octets[2], &octets[3]) == 4) {
        ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    }
    
    return ip;
}

int network_mac_equal(uint8_t* mac1, uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

void network_copy_mac(uint8_t* dest, uint8_t* src) {
    memcpy(dest, src, 6);
}

// RTL8139 Hardware Functions
int rtl8139_init(network_driver_t* driver) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    if (!rtl) return -1;
    
    printf("RTL8139 baÅŸlatÄ±lÄ±yor...\n");
    
    // Reset kartÄ±
    outb(rtl->io_base + RTL8139_REG_CMD, RTL8139_CMD_RESET);
    while (inb(rtl->io_base + RTL8139_REG_CMD) & RTL8139_CMD_RESET) {
        // Reset bit'i temizlenene kadar bekle
    }
    
    // MAC adresini oku
    uint32_t mac_low = inl(rtl->io_base + RTL8139_REG_MAC0_3);
    uint16_t mac_high = inw(rtl->io_base + RTL8139_REG_MAC4_5);
    
    rtl->base.mac_address[0] = mac_low & 0xFF;
    rtl->base.mac_address[1] = (mac_low >> 8) & 0xFF;
    rtl->base.mac_address[2] = (mac_low >> 16) & 0xFF;
    rtl->base.mac_address[3] = (mac_low >> 24) & 0xFF;
    rtl->base.mac_address[4] = mac_high & 0xFF;
    rtl->base.mac_address[5] = (mac_high >> 8) & 0xFF;
    
    char mac_str[18];
    network_format_mac(rtl->base.mac_address, mac_str);
    printf("RTL8139 MAC adresi: %s\n", mac_str);
    
    // TX buffer'larÄ± ayarla
    for (int i = 0; i < 4; i++) {
        rtl->tx_buffers[i] = malloc(1518);
        outl(rtl->io_base + RTL8139_REG_TX_ADDR0 + i * 4, (uint32_t)rtl->tx_buffers[i]);
    }
    
    // RX buffer'Ä± ayarla
    rtl->rx_buffer_size = 8192 + 16;
    rtl->rx_buffer = malloc(rtl->rx_buffer_size);
    outl(rtl->io_base + RTL8139_REG_RBSTART, (uint32_t)rtl->rx_buffer);
    
    // RX konfigÃ¼rasyonu
    outl(rtl->io_base + RTL8139_REG_RX_CONFIG, 0xF | (1 << 7)); // Accept all packets
    
    // Interrupt'larÄ± etkinleÅŸtir
    outw(rtl->io_base + RTL8139_REG_IMR, 0x0005); // TX OK + RX OK
    
    // TX ve RX'i etkinleÅŸtir
    uint8_t cmd = inb(rtl->io_base + RTL8139_REG_CMD);
    cmd |= RTL8139_CMD_TX_ENABLE | RTL8139_CMD_RX_ENABLE;
    outb(rtl->io_base + RTL8139_REG_CMD, cmd);
    
    rtl->current_tx_buffer = 0;
    
    rtl8139_initialized = 1;
    return 0;
}

int rtl8139_send(network_driver_t* driver, uint8_t* data, uint32_t size) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    if (!rtl8139_initialized || !data || size > 1518) {
        return -1;
    }
    
    // TX buffer'Ä±na kopyala
    memcpy(rtl->tx_buffers[rtl->current_tx_buffer], data, size);
    
    // TX status'u ayarla (packet length + send bit)
    outl(rtl->io_base + RTL8139_REG_TX_ADDR0 + rtl->current_tx_buffer * 4, 
         (uint32_t)rtl->tx_buffers[rtl->current_tx_buffer]);
    
    // Packet length'i ayarla
    outl(rtl->io_base + RTL8139_REG_TX_CONFIG + rtl->current_tx_buffer * 4, size);
    
    // Sonraki buffer'a geÃ§
    rtl->current_tx_buffer = (rtl->current_tx_buffer + 1) % 4;
    
    return 0;
}

int rtl8139_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size) {
    rtl8139_driver_t* rtl = (rtl8139_driver_t*)driver;
    
    if (!rtl8139_initialized || !buffer || !size) {
        return -1;
    }
    
    // Current buffer pointer'Ä± al
    uint16_t capr = inw(rtl->io_base + RTL8139_REG_CAPR);
    uint16_t cbr = inw(rtl->io_base + RTL8139_REG_CBR);
    
    if (capr == cbr) {
        return -1; // No packets available
    }
    
    // Packet header'Ä± oku
    uint8_t* rx_ptr = rtl->rx_buffer + capr;
    uint16_t packet_length = *(uint16_t*)(rx_ptr + 2);
    uint16_t packet_status = *(uint16_t*)rx_ptr;
    
    // Packet'i kopyala
    if (packet_length <= *size) {
        memcpy(buffer, rx_ptr + 4, packet_length - 4); // -4 for CRC
        *size = packet_length - 4;
    } else {
        return -1; // Buffer too small
    }
    
    // Update CAPR
    capr = (capr + 4 + packet_length + 3) & ~3; // Align to 4 bytes
    outw(rtl->io_base + RTL8139_REG_CAPR, capr);
    
    return 0;
}

// Intel E1000 Hardware Functions (simplified)
int e1000_init(network_driver_t* driver) {
    e1000_driver_t* e1000 = (e1000_driver_t*)driver;
    
    if (!e1000) return -1;
    
    printf("Intel E1000 baÅŸlatÄ±lÄ±yor...\n");
    
    // Reset
    uint32_t ctrl = inl(e1000->mmio_base + E1000_REG_CTRL);
    ctrl |= 0x04000000; // Reset bit
    outl(e1000->mmio_base + E1000_REG_CTRL, ctrl);
    
    // Reset'in bitmesini bekle
    while (inl(e1000->mmio_base + E1000_REG_CTRL) & 0x04000000) {
        // Bekle
    }
    
    // MAC adresini oku
    e1000->mac_addr_low = inl(e1000->mmio_base + E1000_REG_RAL);
    e1000->mac_addr_high = inl(e1000->mmio_base + E1000_REG_RAH);
    
    e1000->base.mac_address[0] = e1000->mac_addr_low & 0xFF;
    e1000->base.mac_address[1] = (e1000->mac_addr_low >> 8) & 0xFF;
    e1000->base.mac_address[2] = (e1000->mac_addr_low >> 16) & 0xFF;
    e1000->base.mac_address[3] = (e1000->mac_addr_low >> 24) & 0xFF;
    e1000->base.mac_address[4] = e1000->mac_addr_high & 0xFF;
    e1000->base.mac_address[5] = (e1000->mac_addr_high >> 8) & 0xFF;
    
    char mac_str[18];
    network_format_mac(e1000->base.mac_address, mac_str);
    printf("E1000 MAC adresi: %s\n", mac_str);
    
    e1000_initialized = 1;
    return 0;
}

int e1000_send(network_driver_t* driver, uint8_t* data, uint32_t size) {
    // E1000 send implementasyonu
    return 0;
}

int e1000_receive(network_driver_t* driver, uint8_t* buffer, uint32_t* size) {
    // E1000 receive implementasyonu
    return 0;
}

driver_t* create_rtl8139_driver(pci_device_t* device) {
    if (rtl8139_initialized) {
        printf("RTL8139 zaten baÅŸlatÄ±lmÄ±ÅŸ\n");
        return &rtl8139_driver.base.base;
    }
    
    // I/O base adresini al
    uint32_t io_base = device ? device->bar[0] : 0;
    if (io_base & 0x01) {
        rtl8139_driver.io_base = io_base & ~0x01;
    } else {
        printf("RTL8139: Memory mapped I/O desteklenmiyor\n");
        return NULL;
    }
    
    // SÃ¼rÃ¼cÃ¼ yapÄ±sÄ±nÄ± ayarla
    strcpy(rtl8139_driver.base.base.name, "RTL8139 Ethernet");
    rtl8139_driver.base.base.type = DRIVER_TYPE_NET;
    rtl8139_driver.base.base.class = DRIVER_CLASS_NETWORK;
    rtl8139_driver.base.base.vendor_id = device ? device->vendor_id : 0x10EC;
    rtl8139_driver.base.base.device_id = device ? device->device_id : 0x8139;
    rtl8139_driver.base.base.init = rtl8139_driver_init;
    rtl8139_driver.base.base.read = rtl8139_driver_read;
    rtl8139_driver.base.base.write = rtl8139_driver_write;
    rtl8139_driver.base.base.ioctl = rtl8139_driver_ioctl;
    rtl8139_driver.base.base.shutdown = rtl8139_driver_shutdown;
    
    printf("RTL8139 sÃ¼rÃ¼cÃ¼sÃ¼ oluÅŸturuldu, I/O base: 0x%04X (%04X:%04X)\n", 
           rtl8139_driver.io_base, 
           device ? device->vendor_id : 0x10EC, 
           device ? device->device_id : 0x8139);
    return &rtl8139_driver.base.base;
}

driver_t* create_e1000_driver(pci_device_t* device) {
    if (e1000_initialized) {
        printf("Intel E1000 zaten baÅŸlatÄ±lmÄ±ÅŸ\n");
        return &e1000_driver.base.base;
    }
    
    // MMIO base adresini al
    uint32_t mmio_base = device ? device->bar[0] : 0;
    if (!(mmio_base & 0x01)) {
        e1000_driver.mmio_base = mmio_base;
    } else {
        printf("E1000: I/O mapped I/O desteklenmiyor\n");
        return NULL;
    }
    
    // SÃ¼rÃ¼cÃ¼ yapÄ±sÄ±nÄ± ayarla
    strcpy(e1000_driver.base.base.name, "Intel E1000 Ethernet");
    e1000_driver.base.base.type = DRIVER_TYPE_NET;
    e1000_driver.base.base.class = DRIVER_CLASS_NETWORK;
    e1000_driver.base.base.vendor_id = device ? device->vendor_id : 0x8086;
    e1000_driver.base.base.device_id = device ? device->device_id : 0x1000;
    e1000_driver.base.base.init = e1000_driver_init;
    e1000_driver.base.base.read = e1000_driver_read;
    e1000_driver.base.base.write = e1000_driver_write;
    e1000_driver.base.base.ioctl = e1000_driver_ioctl;
    e1000_driver.base.base.shutdown = e1000_driver_shutdown;
    
    printf("Intel E1000 sÃ¼rÃ¼cÃ¼sÃ¼ oluÅŸturuldu, MMIO base: 0x%08X (%04X:%04X)\n", 
           e1000_driver.mmio_base,
           device ? device->vendor_id : 0x8086, 
           device ? device->device_id : 0x1000);
    return &e1000_driver.base.base;
}
