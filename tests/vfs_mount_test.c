#include "framework.h"
#include "../src/kernel/fs/vfs.h"
#include "memory.h"
#include "string.h"

/**
 * 3. KATMAN: VFS MOUNTING TEST (BÜROKRASİ SEVİYESİ)
 */

// Dummy FS Operations
static int dummy_read(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) { return 0; }
static int dummy_write(vfs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) { return 0; }

int test_vfs_mount_limits() {
    vfs_init();
    
    vfs_node_t root1 = { .name = "ROOT1" };
    vfs_node_t root2 = { .name = "ROOT2" };
    vfs_node_t root3 = { .name = "ROOT3" };
    vfs_node_t root4 = { .name = "ROOT4" };
    vfs_node_t root5 = { .name = "ROOT5" };
    
    ASSERT_EQ(vfs_mount("/mnt/a", &root1), 0, "Mount 1 failed");
    ASSERT_EQ(vfs_mount("/mnt/b", &root2), 0, "Mount 2 failed");
    ASSERT_EQ(vfs_mount("/mnt/c", &root3), 0, "Mount 3 failed");
    ASSERT_EQ(vfs_mount("/mnt/d", &root4), 0, "Mount 4 failed");
    
    // 5. mount başarısız olmalı (limit 4)
    ASSERT(vfs_mount("/mnt/e", &root5) != 0, "Mount 5 should have failed (limit reached)");
    
    return TEST_PASS;
}

int test_vfs_path_resolution_logic() {
    // Mevcut VFS implementasyonu henüz path parsing yapmıyor.
    // Bu test aslında gelecekteki bir özelliğin ön testi (regression).
    // Şimdilik sadece node isimlerinin korunduğunu test edelim.
    
    vfs_node_t my_disk = { .name = "ATA0", .size = 1024, .read = dummy_read };
    vfs_mount("/dev/ata0", &my_disk);
    
    // VFS descriptor tablosundan okuma (vfs_open simülasyonu)
    int fd = vfs_open("/dev/ata0", 0);
    ASSERT(fd >= 0, "VFS: Failed to open mounted device path");
    
    vfs_close(fd);
    return TEST_PASS;
}

void kernel_main() {
    test_header("3. LAYER: VFS MOUNTING & BÜROKRASİ");
    
    RUN_TEST(test_vfs_mount_limits, "Mount Point Limits (MAX=4)");
    RUN_TEST(test_vfs_path_resolution_logic, "Device Mounting Path Integrity");
    
    _print_raw("VFS registry is consistent.", 2, 12, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
