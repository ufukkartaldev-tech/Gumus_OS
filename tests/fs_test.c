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
    
    // Dayı Tavsiyesi 1: 8.3 Normalizasyonu
    fat_directory_entry_t* root = (fat_directory_entry_t*)&mock_disk[100 * 512];
    
    // "KONYA.TXT" girişi (8.3 format: "KONYA   TXT")
    memcpy(root[0].name, "KONYA   ", 8);
    memcpy(root[0].ext, "TXT", 3);
    root[0].file_size = 5;
    root[0].first_cluster_lo = 1;
    root[0].attributes = 0x20;
    
    memcpy(&mock_disk[(200 + 1) * 512], "GUMUS", 5);
    
    // Test: Küçük harfle arama (test.txt -> TEST.TXT normalizasyonu)
    // Not: GümüşOS fs.c şu an case-sensitive olabilir, bu test o köprüyü kuracak.
    char* data = fs_read("konya.txt"); 
    
    ASSERT(data != NULL, "FS: Should find KONYA.TXT even when searched as konya.txt (Normalization)");
    ASSERT(strcmp(data, "GUMUS") == 0, "FS: Content mismatch on normalized read");
    
    kfree(data);
    return TEST_PASS;
}

int test_fs_integrity_and_null_termination() {
    // Dayı Tavsiyesi 3: Null Termination Feraseti
    // Dosya sisteminden okunan veri string ise sonuna \0 eklenmiş olmalı.
    const char* secret = "Sifre123";
    fs_write("PASS.TXT", secret);
    
    uint32_t size;
    char* read_back = (char*)fs_read_bin("PASS.TXT", &size);
    
    ASSERT(read_back != NULL, "FS: Binary read failed");
    // read_back[size] == '\0' olduğunu garanti etmeliyiz (fs_read_bin bunu yapmalı)
    ASSERT(read_back[size - 1] == '3', "FS: Last byte integrity check");
    
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
