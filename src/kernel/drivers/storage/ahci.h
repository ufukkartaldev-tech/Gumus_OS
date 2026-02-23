#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>
#include "driver.h"
#include "hardware_detect.h"

// AHCI Sabitleri
#define AHCI_CAP_S64A     (1 << 31)
#define AHCI_CAP_NCQ      (1 << 30)
#define AHCI_CAP_SNTF     (1 << 29)
#define AHCI_CAP_PM       (1 << 28)
#define AHCI_CAP_CCCS     (1 << 27)
#define AHCI_CAP_EMS      (1 << 26)
#define AHCI_CAP_SXS      (1 << 25)
#define AHCI_CAP_NCS      0x1F000000
#define AHCI_CAP_NCS_SHIFT 8
#define AHCI_CAP_PSC      (1 << 7)
#define AHCI_CAP_SSC      (1 << 6)
#define AHCI_CAP_PMD      (1 << 5)
#define AHCI_CAP_FBS      (1 << 4)
#define AHCI_CAP_SPM      (1 << 3)
#define AHCI_CAP_SAM      (1 << 2)
#define AHCI_CAP_PI       (1 << 1)
#define AHCI_CAP_SSS      (1 << 0)

// HBA Port Sabitleri
#define HBA_PxCMD_ST       0x0001
#define HBA_PxCMD_FRE      0x0010
#define HBA_PxCMD_FR       0x4000
#define HBA_PxCMD_CR       0x8000

// FIS TÃ¼rleri
#define FIS_TYPE_REG_H2D   0x27
#define FIS_TYPE_REG_D2H   0x34
#define FIS_TYPE_DMA_ACT   0x39
#define FIS_TYPE_DMA_SETUP 0x41
#define FIS_TYPE_DATA      0x46
#define FIS_TYPE_BIST      0x58
#define FIS_TYPE_PIO_SETUP 0x5F
#define FIS_TYPE_DEV_BITS  0xA1

// ATA KomutlarÄ±
#define ATA_CMD_READ_DMA_EX     0x25
#define ATA_CMD_WRITE_DMA_EX    0x35
#define ATA_CMD_IDENTIFY_DEVICE 0xEC

// AHCI YapÄ±larÄ±
typedef struct {
    uint32_t clb;     // Command List Base Address
    uint32_t clbu;    // Command List Base Address Upper 32 bits
    uint32_t fb;      // FIS Base Address
    uint32_t fbu;     // FIS Base Address Upper 32 bits
    uint32_t is;      // Interrupt Status
    uint32_t ie;      // Interrupt Enable
    uint32_t cmd;     // Command and Status
    uint32_t rsv0;    // Reserved
    uint32_t tfd;     // Task File Data
    uint32_t sig;     // Signature
    uint32_t ssts;    // SATA Status
    uint32_t sctl;    // SATA Control
    uint32_t serr;    // SATA Error
    uint32_t sact;    // SATA Active
    uint32_t ci;      // Command Issue
    uint32_t sntf;    // SNotification
    uint32_t fbs;     // FIS-based Switching Control
    uint32_t rsv1[11]; // Reserved
    uint32_t vendor[4]; // Vendor Specific
} hba_port_t;

typedef struct {
    uint32_t cap;      // Host Capabilities
    uint32_t ghc;      // Global Host Control
    uint32_t is;       // Interrupt Status
    uint32_t pi;       // Ports Implemented
    uint32_t vs;       // Version
    uint32_t ccc_ctl;  // Command Completion Coalescing Control
    uint32_t ccc_pts;  // Command Completion Coalescing Ports
    uint32_t em_loc;   // Enclosure Management Location
    uint32_t em_ctl;   // Enclosure Management Control
    uint32_t cap2;     // Host Capabilities Extended
    uint32_t bohc;     // BIOS/OS Handoff Control and Status
    uint8_t  rsv0[0x74 - 0x2C];
    uint8_t  vendor[0x80 - 0x74];
    hba_port_t ports[32]; // 1-32 Ports
} hba_mem_t;

// Command List Structure
typedef struct {
    uint8_t  cfl;      // Command FIS Length
    uint8_t  a;        // ATAPI
    uint8_t  w;        // Write
    uint8_t  p;        // Prefetchable
    uint8_t  r;        // Reset
    uint8_t  b;        // BIST
    uint8_t  c;        // Clear Busy upon R_OK
    uint8_t  rsv0;     // Reserved
    uint8_t  pmp;      // Port Multiplier Port
    uint16_t prdtl;    // Physical Region Descriptor Table Length
    uint32_t prdbc;    // Physical Region Descriptor Byte Count
    uint64_t ctba;     // Command Table Descriptor Base Address
    uint32_t ctbau;    // Command Table Descriptor Base Address Upper 32 bits
    uint32_t rsv1[4];  // Reserved
} hba_cmd_header_t;

// FIS Structure
typedef struct {
    uint8_t  fis_type;  // FIS Type
    uint8_t  pm;        // Port Multiplier
    uint8_t  rsv0;      // Reserved
    uint8_t  c;         // Command Register
    uint8_t  feature;   // Feature Register
    uint8_t  lba0;      // LBA Low
    uint8_t  lba1;      // LBA Mid
    uint8_t  lba2;      // LBA High
    uint8_t  device;    // Device Register
    uint8_t  lba3;      // LBA 3
    uint8_t  lba4;      // LBA 4
    uint8_t  lba5;      // LBA 5
    uint8_t  feature7;  // Feature 7
    uint8_t  count;     // Count Register
    uint8_t  count7;    // Count 7
    uint8_t  icc;       // Isochronous Command Completion
    uint8_t  command;   // Command Register
    uint8_t  control;   // Control Register
    uint8_t  rsv1[4];   // Reserved
} hba_fis_reg_h2d_t;

// Physical Region Descriptor
typedef struct {
    uint32_t dba;      // Data Base Address
    uint32_t dbau;     // Data Base Address Upper 32 bits
    uint32_t rsv0;     // Reserved
    uint32_t dbc;      // Byte Count - 1
    uint32_t rsv1;     // Reserved
    uint32_t i;        // Interrupt on Completion
} hba_prdt_t;

// AHCI SÃ¼rÃ¼cÃ¼ YapÄ±sÄ±
typedef struct {
    driver_t base;
    hba_mem_t* hba;
    hba_port_t* port;
    uint32_t port_num;
    uint8_t* command_list;
    uint8_t* fis;
    uint8_t* command_table;
    int initialized;
} ahci_driver_t;

// AHCI FonksiyonlarÄ±
int ahci_init();
int ahci_read(ahci_driver_t* driver, void* buffer, uint64_t lba, uint32_t count);
int ahci_write(ahci_driver_t* driver, void* buffer, uint64_t lba, uint32_t count);
int ahci_identify(ahci_driver_t* driver);
int ahci_port_type(hba_port_t* port);
void ahci_port_rebase(hba_port_t* port, int port_num);
int ahci_find_command_slot(hba_port_t* port);
int ahci_issue_command(hba_port_t* port, int slot, hba_fis_reg_h2d_t* fis, hba_prdt_t* prdt, int prdt_count);

// SÃ¼rÃ¼cÃ¼ OluÅŸturma Fonksiyonu
driver_t* create_ahci_driver(pci_device_t* device);

#endif
