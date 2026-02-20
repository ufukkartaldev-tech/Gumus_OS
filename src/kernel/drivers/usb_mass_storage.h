#ifndef USB_MASS_STORAGE_H
#define USB_MASS_STORAGE_H

#include "usb_host.h"
#include "../core/types.h"

// USB Mass Storage Class Constants
#define USB_MASS_STORAGE_CLASS    0x08
#define USB_MASS_STORAGE_SUBCLASS_RBC  0x01
#define USB_MASS_STORAGE_SUBCLASS_SFF8020I 0x02
#define USB_MASS_STORAGE_SUBCLASS_QIC157 0x03
#define USB_MASS_STORAGE_SUBCLASS_UFI 0x04
#define USB_MASS_STORAGE_SUBCLASS_SFF8070I 0x05
#define USB_MASS_STORAGE_SUBCLASS_SCSI 0x06

// USB Mass Storage Protocols
#define USB_MASS_STORAGE_PROTOCOL_CBI 0x00
#define USB_MASS_STORAGE_PROTOCOL_CBI_NO_INT 0x01
#define USB_MASS_STORAGE_PROTOCOL_BULK_ONLY 0x50

// USB Mass Storage Requests
#define USB_MASS_STORAGE_RESET     0xFF
#define USB_MASS_STORAGE_GET_MAX_LUN 0xFE

// Bulk-Only Transport Commands
#define CBW_SIGNATURE              0x43425355  // "USBC"
#define CSW_SIGNATURE              0x53425355  // "USBS"

// Command Block Wrapper (CBW)
typedef struct {
    uint32_t signature;           // "USBC"
    uint32_t tag;                 // Unique tag
    uint32_t data_transfer_length; // Data length
    uint8_t flags;                // Direction flags
    uint8_t lun;                  // LUN number
    uint8_t cb_length;            // Command block length
    uint8_t cb[16];               // Command block
} __attribute__((packed)) usb_cbw_t;

// Command Status Wrapper (CSW)
typedef struct {
    uint32_t signature;           // "USBS"
    uint32_t tag;                 // Tag from CBW
    uint32_t data_residue;        // Residual data
    uint8_t status;               // Command status
} __attribute__((packed)) usb_csw_t;

// CBW Flags
#define CBW_FLAG_DATA_OUT         0x00
#define CBW_FLAG_DATA_IN          0x80

// CSW Status
#define CSW_STATUS_PASSED         0x00
#define CSW_STATUS_FAILED         0x01
#define CSW_STATUS_PHASE_ERROR    0x02

// SCSI Commands
#define SCSI_TEST_UNIT_READY      0x00
#define SCSI_REQUEST_SENSE        0x03
#define SCSI_INQUIRY              0x12
#define SCSI_READ_CAPACITY_10      0x25
#define SCSI_READ_10              0x28
#define SCSI_WRITE_10             0x2A

// SCSI Inquiry Data
typedef struct {
    uint8_t peripheral_device_type;
    uint8_t removable;
    uint8_t version;
    uint8_t response_data_format;
    uint8_t additional_length;
    uint8_t reserved[3];
    uint8_t vendor_id[8];
    uint8_t product_id[16];
    uint8_t product_revision[4];
} __attribute__((packed)) scsi_inquiry_t;

// SCSI Request Sense Data
typedef struct {
    uint8_t response_code;
    uint8_t obsolete;
    uint8_t sense_key;
    uint8_t information[4];
    uint8_t additional_sense_length;
    uint8_t command_specific[4];
    uint8_t additional_sense_code;
    uint8_t additional_sense_qualifier;
    uint8_t field_replaceable_unit_code;
    uint8_t sense_key_specific[3];
} __attribute__((packed)) scsi_request_sense_t;

// SCSI Read Capacity Data
typedef struct {
    uint32_t last_logical_block_address;
    uint32_t block_length;
} __attribute__((packed)) scsi_read_capacity_t;

// USB Mass Storage Device Structure
typedef struct usb_mass_storage_device {
    usb_device_t* usb_device;
    
    // Endpoints
    uint8_t bulk_in_endpoint;
    uint8_t bulk_out_endpoint;
    uint16_t max_packet_size_in;
    uint16_t max_packet_size_out;
    
    // Device information
    scsi_inquiry_t inquiry;
    scsi_read_capacity_t capacity;
    uint32_t num_blocks;
    uint32_t block_size;
    
    // State
    uint8_t lun_count;
    uint32_t current_tag;
    uint8_t ready;
    
    struct usb_mass_storage_device* next;
} usb_mass_storage_device_t;

// USB Mass Storage Driver Structure
typedef struct {
    driver_t base;
    usb_mass_storage_device_t* devices;
    uint32_t device_count;
} usb_mass_storage_driver_t;

// Function Prototypes
int usb_mass_storage_init();
int usb_mass_storage_probe(usb_device_t* usb_device);
int usb_mass_storage_remove(usb_device_t* usb_device);
int usb_mass_storage_read(usb_mass_storage_device_t* device, uint32_t lba, 
                          uint8_t* buffer, uint32_t block_count);
int usb_mass_storage_write(usb_mass_storage_device_t* device, uint32_t lba, 
                           const uint8_t* buffer, uint32_t block_count);
int usb_mass_storage_test_unit_ready(usb_mass_storage_device_t* device);
int usb_mass_storage_inquiry(usb_mass_storage_device_t* device, scsi_inquiry_t* inquiry);
int usb_mass_storage_request_sense(usb_mass_storage_device_t* device, scsi_request_sense_t* sense);
int usb_mass_storage_read_capacity(usb_mass_storage_device_t* device, scsi_read_capacity_t* capacity);

// Bulk-Only Transport functions
int usb_mass_storage_send_cbw(usb_mass_storage_device_t* device, uint32_t tag, 
                             uint32_t data_length, uint8_t flags, uint8_t lun, 
                             uint8_t cb_length, const uint8_t* cb);
int usb_mass_storage_get_csw(usb_mass_storage_device_t* device, uint32_t tag, usb_csw_t* csw);
int usb_mass_storage_reset(usb_mass_storage_device_t* device);
int usb_mass_storage_get_max_lun(usb_mass_storage_device_t* device, uint8_t* max_lun);

// Device management
usb_mass_storage_device_t* usb_mass_storage_find_device(usb_device_t* usb_device);
int usb_mass_storage_add_device(usb_mass_storage_device_t* device);
int usb_mass_storage_remove_device(usb_mass_storage_device_t* device);
void usb_mass_storage_list_devices();

// Driver interface functions
driver_t* create_usb_mass_storage_driver();

#endif
