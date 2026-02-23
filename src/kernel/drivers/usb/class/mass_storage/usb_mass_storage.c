#include "usb_mass_storage.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"

// Global mass storage driver
static usb_mass_storage_driver_t mass_storage_driver;

// Global device list
static usb_mass_storage_device_t* mass_storage_devices = NULL;
static uint32_t next_tag = 1;

// Initialize USB Mass Storage driver
int usb_mass_storage_init() {
    printf("USB Mass Storage: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±lÄ±yor\n");
    
    mass_storage_driver.devices = NULL;
    mass_storage_driver.device_count = 0;
    
    strcpy(mass_storage_driver.base.name, "USB Mass Storage Driver");
    mass_storage_driver.base.type = DRIVER_TYPE_BLOCK;
    mass_storage_driver.base.class = DRIVER_CLASS_STORAGE;
    mass_storage_driver.base.vendor_id = 0;
    mass_storage_driver.base.device_id = 0;
    
    printf("USB Mass Storage: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±ldÄ±\n");
    return 0;
}

// Send Command Block Wrapper (CBW)
int usb_mass_storage_send_cbw(usb_mass_storage_device_t* device, uint32_t tag, 
                             uint32_t data_length, uint8_t flags, uint8_t lun, 
                             uint8_t cb_length, const uint8_t* cb) {
    usb_cbw_t cbw;
    
    // Setup CBW
    cbw.signature = CBW_SIGNATURE;
    cbw.tag = tag;
    cbw.data_transfer_length = data_length;
    cbw.flags = flags;
    cbw.lun = lun;
    cbw.cb_length = cb_length;
    
    // Copy command block
    memset(cbw.cb, 0, sizeof(cbw.cb));
    memcpy(cbw.cb, cb, cb_length);
    
    // Send CBW to device
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_out_endpoint,
        (uint8_t*)&cbw,
        sizeof(cbw),
        USB_DIRECTION_OUT
    );
    
    if (result != 0) {
        printf("USB Mass Storage: CBW gÃ¶nderme baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    return 0;
}

// Get Command Status Wrapper (CSW)
int usb_mass_storage_get_csw(usb_mass_storage_device_t* device, uint32_t tag, usb_csw_t* csw) {
    // Receive CSW from device
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_in_endpoint,
        (uint8_t*)csw,
        sizeof(usb_csw_t),
        USB_DIRECTION_IN
    );
    
    if (result != 0) {
        printf("USB Mass Storage: CSW alma baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Verify CSW signature and tag
    if (csw->signature != CSW_SIGNATURE) {
        printf("USB Mass Storage: GeÃ§ersiz CSW imzasÄ±: 0x%08X\n", csw->signature);
        return -1;
    }
    
    if (csw->tag != tag) {
        printf("USB Mass Storage: CSW tag eÅŸleÅŸmiyor: beklenen=0x%08X, alÄ±nan=0x%08X\n", 
               tag, csw->tag);
        return -1;
    }
    
    return 0;
}

// Reset mass storage device
int usb_mass_storage_reset(usb_mass_storage_device_t* device) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0x21; // Host-to-device, Class, Interface
    setup.request = USB_MASS_STORAGE_RESET;
    setup.value = 0;
    setup.index = 0; // Interface number
    setup.length = 0;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        NULL,
        0
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Reset baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    printf("USB Mass Storage: AygÄ±t resetlendi\n");
    return 0;
}

// Get Max LUN
int usb_mass_storage_get_max_lun(usb_mass_storage_device_t* device, uint8_t* max_lun) {
    usb_setup_packet_t setup;
    
    setup.request_type = 0xA1; // Device-to-host, Class, Interface
    setup.request = USB_MASS_STORAGE_GET_MAX_LUN;
    setup.value = 0;
    setup.index = 0; // Interface number
    setup.length = 1;
    
    int result = usb_host_control_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        0,
        &setup,
        max_lun,
        1
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Max LUN alma baÅŸarÄ±sÄ±z, varsayÄ±lan 0 kullanÄ±lÄ±yor\n");
        *max_lun = 0;
        return 0; // Not a fatal error
    }
    
    printf("USB Mass Storage: Max LUN: %d\n", *max_lun);
    return 0;
}

// Test Unit Ready
int usb_mass_storage_test_unit_ready(usb_mass_storage_device_t* device) {
    uint8_t cb[6] = {SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0};
    usb_csw_t csw;
    uint32_t tag = next_tag++;
    
    // Send CBW
    if (usb_mass_storage_send_cbw(device, tag, 0, CBW_FLAG_DATA_OUT, 0, 6, cb) != 0) {
        return -1;
    }
    
    // Get CSW
    if (usb_mass_storage_get_csw(device, tag, &csw) != 0) {
        return -1;
    }
    
    if (csw.status != CSW_STATUS_PASSED) {
        printf("USB Mass Storage: Test Unit Ready baÅŸarÄ±sÄ±z, status: %d\n", csw.status);
        return -1;
    }
    
    return 0;
}

// Inquiry
int usb_mass_storage_inquiry(usb_mass_storage_device_t* device, scsi_inquiry_t* inquiry) {
    uint8_t cb[6] = {SCSI_INQUIRY, 0, 0, 0, sizeof(scsi_inquiry_t), 0};
    usb_csw_t csw;
    uint32_t tag = next_tag++;
    
    // Send CBW
    if (usb_mass_storage_send_cbw(device, tag, sizeof(scsi_inquiry_t), 
                                  CBW_FLAG_DATA_IN, 0, 6, cb) != 0) {
        return -1;
    }
    
    // Receive inquiry data
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_in_endpoint,
        (uint8_t*)inquiry,
        sizeof(scsi_inquiry_t),
        USB_DIRECTION_IN
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Inquiry data alma baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Get CSW
    if (usb_mass_storage_get_csw(device, tag, &csw) != 0) {
        return -1;
    }
    
    if (csw.status != CSW_STATUS_PASSED) {
        printf("USB Mass Storage: Inquiry baÅŸarÄ±sÄ±z, status: %d\n", csw.status);
        return -1;
    }
    
    return 0;
}

// Read Capacity
int usb_mass_storage_read_capacity(usb_mass_storage_device_t* device, scsi_read_capacity_t* capacity) {
    uint8_t cb[10] = {SCSI_READ_CAPACITY_10, 0, 0, 0, 0, 0, 0, 0, 8, 0};
    usb_csw_t csw;
    uint32_t tag = next_tag++;
    
    // Send CBW
    if (usb_mass_storage_send_cbw(device, tag, sizeof(scsi_read_capacity_t), 
                                  CBW_FLAG_DATA_IN, 0, 10, cb) != 0) {
        return -1;
    }
    
    // Receive capacity data
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_in_endpoint,
        (uint8_t*)capacity,
        sizeof(scsi_read_capacity_t),
        USB_DIRECTION_IN
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Capacity data alma baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Get CSW
    if (usb_mass_storage_get_csw(device, tag, &csw) != 0) {
        return -1;
    }
    
    if (csw.status != CSW_STATUS_PASSED) {
        printf("USB Mass Storage: Read Capacity baÅŸarÄ±sÄ±z, status: %d\n", csw.status);
        return -1;
    }
    
    return 0;
}

// Read blocks
int usb_mass_storage_read(usb_mass_storage_device_t* device, uint32_t lba, 
                          uint8_t* buffer, uint32_t block_count) {
    uint8_t cb[10] = {SCSI_READ_10, 0, 
                      (lba >> 24) & 0xFF, (lba >> 16) & 0xFF, 
                      (lba >> 8) & 0xFF, lba & 0xFF,
                      0, (block_count >> 8) & 0xFF, block_count & 0xFF, 0};
    usb_csw_t csw;
    uint32_t tag = next_tag++;
    uint32_t data_length = block_count * device->block_size;
    
    // Send CBW
    if (usb_mass_storage_send_cbw(device, tag, data_length, 
                                  CBW_FLAG_DATA_IN, 0, 10, cb) != 0) {
        return -1;
    }
    
    // Receive data
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_in_endpoint,
        buffer,
        data_length,
        USB_DIRECTION_IN
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Read data alma baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Get CSW
    if (usb_mass_storage_get_csw(device, tag, &csw) != 0) {
        return -1;
    }
    
    if (csw.status != CSW_STATUS_PASSED) {
        printf("USB Mass Storage: Read baÅŸarÄ±sÄ±z, status: %d\n", csw.status);
        return -1;
    }
    
    return 0;
}

// Write blocks
int usb_mass_storage_write(usb_mass_storage_device_t* device, uint32_t lba, 
                           const uint8_t* buffer, uint32_t block_count) {
    uint8_t cb[10] = {SCSI_WRITE_10, 0, 
                      (lba >> 24) & 0xFF, (lba >> 16) & 0xFF, 
                      (lba >> 8) & 0xFF, lba & 0xFF,
                      0, (block_count >> 8) & 0xFF, block_count & 0xFF, 0};
    usb_csw_t csw;
    uint32_t tag = next_tag++;
    uint32_t data_length = block_count * device->block_size;
    
    // Send CBW
    if (usb_mass_storage_send_cbw(device, tag, data_length, 
                                  CBW_FLAG_DATA_OUT, 0, 10, cb) != 0) {
        return -1;
    }
    
    // Send data
    int result = usb_host_bulk_transfer(
        (usb_host_controller_t*)device->usb_device->host_controller,
        device->usb_device->address,
        device->bulk_out_endpoint,
        (uint8_t*)buffer,
        data_length,
        USB_DIRECTION_OUT
    );
    
    if (result != 0) {
        printf("USB Mass Storage: Write data gÃ¶nderme baÅŸarÄ±sÄ±z\n");
        return -1;
    }
    
    // Get CSW
    if (usb_mass_storage_get_csw(device, tag, &csw) != 0) {
        return -1;
    }
    
    if (csw.status != CSW_STATUS_PASSED) {
        printf("USB Mass Storage: Write baÅŸarÄ±sÄ±z, status: %d\n", csw.status);
        return -1;
    }
    
    return 0;
}

// Probe USB device for mass storage support
int usb_mass_storage_probe(usb_device_t* usb_device) {
    if (!usb_device) {
        return -1;
    }
    
    // Check if this is a mass storage device
    if (usb_device->device_desc.class_code != USB_MASS_STORAGE_CLASS) {
        return -1;
    }
    
    printf("USB Mass Storage: AygÄ±t tespit edildi: VID:PID=%04X:%04X\n",
           usb_device->device_desc.vendor_id, usb_device->device_desc.product_id);
    
    // Create mass storage device structure
    usb_mass_storage_device_t* device = malloc(sizeof(usb_mass_storage_device_t));
    if (!device) {
        printf("USB Mass Storage: Bellek tahsis hatasÄ±\n");
        return -1;
    }
    
    memset(device, 0, sizeof(usb_mass_storage_device_t));
    device->usb_device = usb_device;
    
    // TODO: Find bulk endpoints from interface descriptor
    // For now, use default values
    device->bulk_in_endpoint = 0x81;  // EP1 IN
    device->bulk_out_endpoint = 0x01; // EP1 OUT
    device->max_packet_size_in = 64;
    device->max_packet_size_out = 64;
    
    // Get Max LUN
    uint8_t max_lun;
    usb_mass_storage_get_max_lun(device, &max_lun);
    device->lun_count = max_lun + 1;
    
    // Reset device
    usb_mass_storage_reset(device);
    
    // Test unit ready
    if (usb_mass_storage_test_unit_ready(device) != 0) {
        printf("USB Mass Storage: AygÄ±t hazÄ±r deÄŸil\n");
        free(device);
        return -1;
    }
    
    // Get inquiry data
    if (usb_mass_storage_inquiry(device, &device->inquiry) != 0) {
        printf("USB Mass Storage: Inquiry baÅŸarÄ±sÄ±z\n");
        free(device);
        return -1;
    }
    
    // Get capacity
    if (usb_mass_storage_read_capacity(device, &device->capacity) != 0) {
        printf("USB Mass Storage: Read Capacity baÅŸarÄ±sÄ±z\n");
        free(device);
        return -1;
    }
    
    // Convert capacity data
    device->num_blocks = device->capacity.last_logical_block_address + 1;
    device->block_size = device->capacity.block_length;
    
    device->ready = 1;
    
    // Add to device list
    device->next = mass_storage_devices;
    mass_storage_devices = device;
    mass_storage_driver.device_count++;
    
    printf("USB Mass Storage: AygÄ±t eklendi: %s %s\n", 
           device->inquiry.vendor_id, device->inquiry.product_id);
    printf("  Boyut: %d blok, %d bayt/blok (%.2f MB)\n", 
           device->num_blocks, device->block_size,
           (float)(device->num_blocks * device->block_size) / (1024 * 1024));
    
    return 0;
}

// Remove mass storage device
int usb_mass_storage_remove(usb_device_t* usb_device) {
    usb_mass_storage_device_t** current = &mass_storage_devices;
    
    while (*current) {
        if ((*current)->usb_device == usb_device) {
            usb_mass_storage_device_t* to_remove = *current;
            *current = (*current)->next;
            mass_storage_driver.device_count--;
            
            printf("USB Mass Storage: AygÄ±t kaldÄ±rÄ±ldÄ±: %s %s\n",
                   to_remove->inquiry.vendor_id, to_remove->inquiry.product_id);
            
            free(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    
    return -1;
}

// List all mass storage devices
void usb_mass_storage_list_devices() {
    printf("\n=== USB Mass Storage AygÄ±tlarÄ± ===\n");
    
    usb_mass_storage_device_t* current = mass_storage_devices;
    int count = 1;
    
    while (current) {
        printf("%d. %s %s\n", count++, 
               current->inquiry.vendor_id, current->inquiry.product_id);
        printf("   Boyut: %d blok, %d bayt/blok (%.2f MB)\n", 
               current->num_blocks, current->block_size,
               (float)(current->num_blocks * current->block_size) / (1024 * 1024));
        printf("   LUN SayÄ±sÄ±: %d, HazÄ±r: %s\n", 
               current->lun_count, current->ready ? "Evet" : "HayÄ±r");
        
        current = current->next;
    }
    
    if (count == 1) {
        printf("HiÃ§bir USB Mass Storage aygÄ±tÄ± baÄŸlÄ± deÄŸil\n");
    }
    
    printf("===============================\n");
}

// Find device by USB device
usb_mass_storage_device_t* usb_mass_storage_find_device(usb_device_t* usb_device) {
    usb_mass_storage_device_t* current = mass_storage_devices;
    
    while (current) {
        if (current->usb_device == usb_device) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Create USB Mass Storage driver
driver_t* create_usb_mass_storage_driver() {
    if (usb_mass_storage_init() != 0) {
        return NULL;
    }
    
    return (driver_t*)&mass_storage_driver;
}
