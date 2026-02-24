#include "framework.h"
#include "../src/kernel/fs/fs.h"
#include "string.h"
#include "memory.h"

/**
 * 2. KATMAN: FAT32 / FILE SYSTEM TEST (MANTIKSAL SEVİYE)
 */

// Sahte Disk İmajı (128KB - Root Dir + Data)
static uint8_t mock_disk[128 * 1024];

// ATA Mockları
void ata_read_sectors(uint32_t target_address, uint32_t LBA, uint8_t sector_count) {
    memcpy((void*)target_address, &mock_disk[LBA * 512], sector_count * 512);
}

void ata_write_sectors(uint32_t LBA, uint8_t sector_count, uint32_t source_address) {
    memcpy(&mock_disk[LBA * 512], (void*)source_address, sector_count * 512);
}

int test_fs_filename_normalization() {
    memset(mock_disk, 0, sizeof(mock_disk));
    
    // Root directory sector (defined as 100 in fs.c)
    fat_directory_entry_t* root = (fat_directory_entry_t*)&mock_disk[100 * 512];
    
    // "TEST.TXT" girişi oluştur
    memcpy(root[0].name, "TEST    ", 8);
    memcpy(root[0].ext, "TXT", 3);
    root[0].file_size = 12;
    root[0].first_cluster_lo = 1; // Sector 201 (since start_sector = 200 + cluster)
    root[0].attributes = 0x20;
    
    // Veriyi yaz
    memcpy(&mock_disk[(200 + 1) * 512], "MERHABA DUNYA", 13);
    
    char* data = fs_read("TEST.TXT");
    ASSERT(data != NULL, "FS: File read failed for TEST.TXT");
    ASSERT(strcmp(data, "MERHABA DUNYA") == 0, "FS: File content mismatch");
    
    kfree(data);
    return TEST_PASS;
}

int test_fs_fragmentation_logic() {
    // Mevcut fs.c çok basit (cluster=sector) olduğu için 
    // fragmented file desteği aslında yok. 
    // Ama biz en azından büyük dosya yazımını test edebiliriz.
    
    const char* large_data = "Bu veri birden fazla sektore yayilacak kadar buyuk olmali... "
                             "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                             "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    
    int success = fs_write("LARGE.DAT", large_data);
    ASSERT(success == 1, "FS: Large file write failed");
    
    char* read_back = fs_read("LARGE.DAT");
    ASSERT(read_back != NULL, "FS: Large file read failed");
    ASSERT(strcmp(read_back, large_data) == 0, "FS: Integrity check failed");
    
    kfree(read_back);
    return TEST_PASS;
}

void kernel_main() {
    test_header("2. LAYER: FAT32 LOGICAL VERIFICATION");
    
    RUN_TEST(test_fs_filename_normalization, "Filename Normalization (TEST.TXT)");
    RUN_TEST(test_fs_fragmentation_logic, "Large File Write/Read Integrity");
    
    _print_raw("Filesystem logic consistent.", 2, 12, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
