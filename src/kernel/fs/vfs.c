#include "vfs.h"
#include "fs.h"
#include "string.h"
#include "memory.h"
#include "ext2.h"

#define MAX_OPEN_FILES 16

static vfs_node_t* mount_points[4]; // 4 Mount point (C:, D: etc.)
static int mount_count = 0;

static struct file_desc {
    vfs_node_t* node;
    uint32_t offset;
    int used;
} file_table[MAX_OPEN_FILES];

void vfs_init() {
    for(int i=0; i<MAX_OPEN_FILES; i++) file_table[i].used = 0;
    fs_init(); // ATA tabanlı ilkel FS başlat
    ext2_init(); // EXT2 filesystem'ni başlat
}

// Wrapper Functions to bridge gap between new VFS and old FS code
// (Temporary adaptation layer)

int vfs_mount(const char* path, vfs_node_t* fs_root) {
    if (mount_count < 4) {
        mount_points[mount_count++] = fs_root;
        return 0; 
    }
    return -1;
}

int vfs_open(const char* path, int flags) {
    // EXT2 filesystem mount edilmişse onu kullan
    if (ext2_main_fs.mounted) {
        int fd = -1;
        for(int i = 0; i < MAX_OPEN_FILES; i++) {
            if (!file_table[i].used) {
                fd = i;
                break;
            }
        }
        
        if (fd == -1) return -1;
        
        // EXT2 dosyasını aç
        ext2_file_t* file = kmalloc(sizeof(ext2_file_t));
        if (!file) return -1;
        
        if (ext2_open_file(&ext2_main_fs, path, flags, file) == 0) {
            // VFS node oluştur
            vfs_node_t* node = kmalloc(sizeof(vfs_node_t));
            strcpy(node->name, path);
            node->size = file->size;
            node->flags = 0x01; // File
            node->impl_data = file;
            
            file_table[fd].node = node;
            file_table[fd].offset = 0;
            file_table[fd].used = 1;
            
            return fd;
        } else {
            kfree(file);
            return -1;
        }
    }
    
    // Fallback to eski FS
    int fd = -1;
    for(int i=0; i<MAX_OPEN_FILES; i++) {
        if (!file_table[i].used) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) return -1;
    
    // Node oluştur (Geçici)
    vfs_node_t* node = kmalloc(sizeof(vfs_node_t));
    strcpy(node->name, path);
    // Boyut vs. fs_read_bin ile ögrenilecek
    
    file_table[fd].node = node;
    file_table[fd].offset = 0;
    file_table[fd].used = 1;
    
    return fd;
}

int vfs_read(int fd, void* buf, int size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].used) return -1;
    
    // EXT2 dosyası ise EXT2 üzerinden oku
    if (file_table[fd].node->impl_data) {
        ext2_file_t* file = (ext2_file_t*)file_table[fd].node->impl_data;
        if (file) {
            return ext2_read_file(file, buf, size);
        }
    }
    
    // Fallback to eski FS
    uint32_t fsize;
    uint8_t* data = fs_read_bin(file_table[fd].node->name, &fsize);
    if (!data) return 0;
    
    int to_read = size;
    if (file_table[fd].offset + size > fsize) {
        to_read = fsize - file_table[fd].offset;
    }
    
    if (to_read > 0) {
        memcpy(buf, data + file_table[fd].offset, to_read);
        file_table[fd].offset += to_read;
    }
    
    kfree(data);
    return to_read;
}

int vfs_write(int fd, void* buf, int size) {
    // Yazma iÅŸlemi daha karmaÅŸÄ±k.
    // Åžimdilik tek seferde yazma destekliyoruz (fs_write_bin)
    // Append desteklemiyoruz.
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].used) return -1;
    
    fs_write_bin(file_table[fd].node->name, (uint8_t*)buf, size);
    return size;
}

void vfs_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES && file_table[fd].used) {
        // EXT2 dosyası ise kapat
        if (file_table[fd].node->impl_data) {
            ext2_file_t* file = (ext2_file_t*)file_table[fd].node->impl_data;
            ext2_close_file(file);
            kfree(file);
        }
        
        kfree(file_table[fd].node);
        file_table[fd].used = 0;
    }
}
