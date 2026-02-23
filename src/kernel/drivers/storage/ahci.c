#include "ahci.h"
#include "io.h"
#include "string.h"
#include "memory.h"
#include "printf.h"
#include "stdio.h"

static ahci_driver_t ahci_drivers[8];
static int ahci_driver_count = 0;

// PCI Vendor ID'leri
#define PCI_VENDOR_INTEL    0x8086
#define PCI_VENDOR_AMD      0x1022
#define PCI_VENDOR_REALTEK  0x10EC

// Intel AHCI Controller ID'leri
#define INTEL_82801IR      0x2922
#define INTEL_82801HM      0x2923
#define INTEL_82801IBM     0x2928
#define INTEL_82801IEM     0x2929
#define INTEL_82801JD      0x3A22
#define INTEL_82801JE      0x3A23
#define INTEL_82801JR      0x3A26
#define INTEL_82801JB      0x3A27
#define INTEL_82801DO      0x3A20
#define INTEL_82801DO_SATA 0x3A21
#define INTEL_82801D0      0x3A00
#define INTEL_82801D1      0x3A01
#define INTEL_82801D2      0x3A02
#define INTEL_82801D3      0x3A03

// AMD AHCI Controller ID'leri
#define AMD_SB600          0x4380
#define AMD_SB700          0x4390
#define AMD_SB800          0x4391
#define AMD_SB900          0x4392
#define AMD_SB950          0x4393

static int ahci_driver_init(void) {
    printf("AHCI sÃ¼rÃ¼cÃ¼sÃ¼ baÅŸlatÄ±lÄ±yor...\n");
    return 0;
}

static int ahci_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    ahci_driver_t* ahci = (ahci_driver_t*)buffer;
    if (!ahci || !ahci->initialized) return -1;
    
    uint64_t lba = offset / 512; // 512 byte sektorler
    uint32_t sectors = (size + 511) / 512;
    
    return ahci_read(ahci, buffer, lba, sectors);
}

static int ahci_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    ahci_driver_t* ahci = (ahci_driver_t*)buffer;
    if (!ahci || !ahci->initialized) return -1;
    
    uint64_t lba = offset / 512;
    uint32_t sectors = (size + 511) / 512;
    
    return ahci_write(ahci, buffer, lba, sectors);
}

static int ahci_driver_ioctl(uint32_t command, void* arg) {
    // IOCTL komutlarÄ± iÃ§in
    return 0;
}

static int ahci_driver_shutdown(void) {
    printf("AHCI sÃ¼rÃ¼cÃ¼sÃ¼ kapatÄ±lÄ±yor...\n");
    return 0;
}

int ahci_init() {
    printf("AHCI baÅŸlatÄ±lÄ±yor...\n");
    return 0;
}

int ahci_port_type(hba_port_t* port) {
    uint32_t ssts = port->ssts;
    
    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;
    
    if (det != 0x03) return -1; // No device
    
    if (ipm != 0x01) return -1; // Not active
    
    uint32_t sig = port->sig;
    
    if (sig == 0x00000101) return 0; // SATA
    if (sig == 0xEB140101) return 1; // SATAPI
    if (sig == 0x96690101) return 2; // Enclosure
    if (sig == 0xC33C0101) return 3; // Port Multiplier
    
    return -1;
}

void ahci_port_rebase(hba_port_t* port, int port_num) {
    // Command list base adresini ayarla
    port->clb = (uint32_t)malloc(1024);
    port->clbu = 0;
    memset((void*)port->clb, 0, 1024);
    
    // FIS base adresini ayarla
    port->fb = (uint32_t)malloc(256);
    port->fbu = 0;
    memset((void*)port->fb, 0, 256);
    
    // Command table iÃ§in alan ayÄ±r
    hba_cmd_header_t* cmd_header = (hba_cmd_header_t*)port->clb;
    for (int i = 0; i < 32; i++) {
        cmd_header[i].prdtl = 8; // Physical Region Descriptor Table Length
        cmd_header[i].ctba = (uint32_t)malloc(256);
        cmd_header[i].ctbau = 0;
        memset((void*)cmd_header[i].ctba, 0, 256);
    }
}

int ahci_find_command_slot(hba_port_t* port) {
    uint32_t slots = (port->sact | port->ci);
    
    for (int i = 0; i < 32; i++) {
        if (!(slots & (1 << i))) {
            return i;
        }
    }
    
    return -1;
}

int ahci_issue_command(hba_port_t* port, int slot, hba_fis_reg_h2d_t* fis, hba_prdt_t* prdt, int prdt_count) {
    hba_cmd_header_t* cmd_header = (hba_cmd_header_t*)port->clb;
    hba_cmd_header_t* cmd = &cmd_header[slot];
    
    // Command header'Ä± ayarla
    cmd->cfl = sizeof(hba_fis_reg_h2d_t) / sizeof(uint32_t);
    cmd->w = 0;
    cmd->prdtl = prdt_count;
    
    // Command table'a FIS ve PRDT'yi kopyala
    uint8_t* cmd_table = (uint8_t*)cmd->ctba;
    memcpy(cmd_table, fis, sizeof(hba_fis_reg_h2d_t));
    
    if (prdt_count > 0) {
        memcpy(cmd_table + 0x80, prdt, sizeof(hba_prdt_t) * prdt_count);
    }
    
    // Komutu issue et
    port->ci |= (1 << slot);
    
    // Komutun bitmesini bekle
    while (port->ci & (1 << slot)) {
        // Bekle
    }
    
    // Hata kontrolÃ¼
    if (port->is & (1 << 30)) {
        port->is = (1 << 30); // Task File Error'Ä± temizle
        return -1;
    }
    
    return 0;
}

int ahci_read(ahci_driver_t* driver, void* buffer, uint64_t lba, uint32_t count) {
    hba_port_t* port = driver->port;
    
    int slot = ahci_find_command_slot(port);
    if (slot == -1) return -1;
    
    // FIS oluÅŸtur
    hba_fis_reg_h2d_t fis;
    memset(&fis, 0, sizeof(fis));
    
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.c = 1; // Command
    fis.command = ATA_CMD_READ_DMA_EX;
    
    fis.lba0 = (lba >> 0) & 0xFF;
    fis.lba1 = (lba >> 8) & 0xFF;
    fis.lba2 = (lba >> 16) & 0xFF;
    fis.lba3 = (lba >> 24) & 0xFF;
    fis.lba4 = (lba >> 32) & 0xFF;
    fis.lba5 = (lba >> 40) & 0xFF;
    
    fis.count = count & 0xFF;
    fis.count7 = (count >> 8) & 0xFF;
    
    // PRDT oluÅŸtur
    hba_prdt_t prdt;
    prdt.dba = (uint32_t)buffer;
    prdt.dbau = 0;
    prdt.rsv0 = 0;
    prdt.dbc = (count * 512) - 1; // Byte count - 1
    prdt.rsv1 = 0;
    prdt.i = 1; // Interrupt on completion
    
    return ahci_issue_command(port, slot, &fis, &prdt, 1);
}

int ahci_write(ahci_driver_t* driver, void* buffer, uint64_t lba, uint32_t count) {
    hba_port_t* port = driver->port;
    
    int slot = ahci_find_command_slot(port);
    if (slot == -1) return -1;
    
    // FIS oluÅŸtur
    hba_fis_reg_h2d_t fis;
    memset(&fis, 0, sizeof(fis));
    
    fis.fis_type = FIS_TYPE_REG_H2D;
    fis.c = 1; // Command
    fis.command = ATA_CMD_WRITE_DMA_EX;
    fis.device = 1; // Write
    
    fis.lba0 = (lba >> 0) & 0xFF;
    fis.lba1 = (lba >> 8) & 0xFF;
    fis.lba2 = (lba >> 16) & 0xFF;
    fis.lba3 = (lba >> 24) & 0xFF;
    fis.lba4 = (lba >> 32) & 0xFF;
    fis.lba5 = (lba >> 40) & 0xFF;
    
    fis.count = count & 0xFF;
    fis.count7 = (count >> 8) & 0xFF;
    
    // PRDT oluÅŸtur
    hba_prdt_t prdt;
    prdt.dba = (uint32_t)buffer;
    prdt.dbau = 0;
    prdt.rsv0 = 0;
    prdt.dbc = (count * 512) - 1; // Byte count - 1
    prdt.rsv1 = 0;
    prdt.i = 1; // Interrupt on completion
    
    return ahci_issue_command(port, slot, &fis, &prdt, 1);
}

int ahci_identify(ahci_driver_t* driver) {
    uint16_t identify_data[256];
    
    if (ahci_read(driver, identify_data, 0, 1) == 0) {
        printf("SATA aygÄ±tÄ± identified\n");
        return 0;
    }
    
    return -1;
}

driver_t* create_ahci_driver(pci_device_t* device) {
    if (ahci_driver_count >= 8) return NULL;
    
    ahci_driver_t* driver = &ahci_drivers[ahci_driver_count++];
    
    // SÃ¼rÃ¼cÃ¼ yapÄ±sÄ±nÄ± ayarla
    strcpy(driver->base.name, "AHCI SATA");
    driver->base.type = DRIVER_TYPE_BLOCK;
    driver->base.class = DRIVER_CLASS_STORAGE;
    driver->base.vendor_id = device ? device->vendor_id : 0xFFFF;
    driver->base.device_id = device ? device->device_id : 0xFFFF;
    driver->base.init = ahci_driver_init;
    driver->base.read = ahci_driver_read;
    driver->base.write = ahci_driver_write;
    driver->base.ioctl = ahci_driver_ioctl;
    driver->base.shutdown = ahci_driver_shutdown;
    
    // PCI aygÄ±tÄ±nÄ± yapÄ±landÄ±r
    if (device) {
        uint32_t bar5 = device->bar[5];
        if (!(bar5 & 0x01)) { // Memory mapped I/O
            driver->hba = (hba_mem_t*)bar5;
        } else {
            printf("AHCI: I/O mapped I/O desteklenmiyor\n");
            return NULL;
        }
        
        // HBA'yÄ± baÅŸlat
        driver->hba->ghc |= (1 << 31); // AHCI Enable
        
        // PortlarÄ± tara
        uint32_t ports_impl = driver->hba->pi;
        for (int i = 0; i < 32; i++) {
            if (ports_impl & (1 << i)) {
                hba_port_t* port = &driver->hba->ports[i];
                
                int port_type = ahci_port_type(port);
                if (port_type == 0) { // SATA
                    driver->port = port;
                    driver->port_num = i;
                    
                    // Port'u yeniden yapÄ±landÄ±r
                    port->cmd &= ~HBA_PxCMD_ST; // Stop command
                    port->cmd &= ~HBA_PxCMD_FRE; // Disable FIS receive
                    
                    while (port->cmd & HBA_PxCMD_CR); // Wait for stop
                    while (port->cmd & HBA_PxCMD_FR); // Wait for FIS receive stop
                    
                    // Port'u yeniden baÅŸlat
                    ahci_port_rebase(port, i);
                    
                    port->cmd |= HBA_PxCMD_FRE; // Enable FIS receive
                    port->cmd |= HBA_PxCMD_ST;  // Start command
                    
                    driver->initialized = 1;
                    
                    printf("AHCI SATA aygÄ±tÄ± bulundu: Port %d (%04X:%04X)\n", i, device->vendor_id, device->device_id);
                    
                    // AygÄ±tÄ± tanÄ±
                    ahci_identify(driver);
                    
                    return (driver_t*)driver;
                }
            }
        }
        
        printf("AHCI: SATA aygÄ±tÄ± bulunamadÄ± (%04X:%04X)\n", device->vendor_id, device->device_id);
    }
    
    return (driver_t*)driver;
}
