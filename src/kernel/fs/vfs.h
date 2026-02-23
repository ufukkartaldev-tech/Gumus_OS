#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

// Dosya Sistemi DÃ¼ÄŸÃ¼mÃ¼ (Inode equivalent)
typedef struct vfs_node {
    char name[32];
    uint32_t size;
    uint32_t flags; // 0x01: File, 0x02: Directory
    
    // Op pointers
    int (*read)(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    int (*write)(struct vfs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
    void (*open)(struct vfs_node* node);
    void (*close)(struct vfs_node* node);
    struct vfs_node* (*finddir)(struct vfs_node* node, char* name);
    
    // Internal driver specific data
    void* impl_data; 
} vfs_node_t;

// VFS Public API
void vfs_init();
int vfs_mount(const char* path, vfs_node_t* fs_root);
int vfs_open(const char* path, int flags); // Returns fd
int vfs_read(int fd, void* buf, int size);
int vfs_write(int fd, void* buf, int size);
void vfs_close(int fd);

#endif
