#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <stddef.h>
#include "ata.h"

// EXT2 Sabitleri
#define EXT2_SUPERBLOCK_OFFSET    1024
#define EXT2_BLOCK_SIZE         1024
#define EXT2_SECTOR_SIZE       512
#define EXT2_SIGNATURE         0xEF53
#define EXT2_ROOT_INODE        2
#define EXT2_MAX_PATH_LEN      4096
#define EXT2_MAX_FILENAME_LEN   255

// EXT2 Sürümleri
#define EXT2_REV0            0
#define EXT2_REV1            1

// Inode Türleri
#define EXT2_INODE_UNKNOWN     0
#define EXT2_INODE_REGULAR     1
#define EXT2_INODE_DIRECTORY   2
#define EXT2_INODE_CHARDEV     3
#define EXT2_INODE_BLOCKDEV    4
#define EXT2_INODE_FIFO        5
#define EXT2_INODE_SYMLINK     6
#define EXT2_INODE_SOCK        7

// Dosya İzinleri
#define EXT2_S_IFMT          0xF000    // Dosya türü maskesi
#define EXT2_S_IFREG         0x8000    // Normal dosya
#define EXT2_S_IFDIR         0x4000    // Dizin
#define EXT2_S_IFLNK         0xA000    // Sembolik link
#define EXT2_S_IFBLK         0x6000    // Block device
#define EXT2_S_IFCHR         0x2000    // Character device
#define EXT2_S_IFIFO         0x1000    // FIFO

#define EXT2_S_ISUID         0x0800    // Set user ID
#define EXT2_S_ISGID         0x0400    // Set group ID
#define EXT2_S_ISVTX         0x0200    // Sticky bit

#define EXT2_S_IRUSR         0x0100    // Owner read
#define EXT2_S_IWUSR         0x0080    // Owner write
#define EXT2_S_IXUSR         0x0040    // Owner execute
#define EXT2_S_IRGRP         0x0020    // Group read
#define EXT2_S_IWGRP         0x0010    // Group write
#define EXT2_S_IXGRP         0x0008    // Group execute
#define EXT2_S_IROTH         0x0004    // Others read
#define EXT2_S_IWOTH         0x0002    // Others write
#define EXT2_S_IXOTH         0x0001    // Others execute

// EXT2 Superblock
typedef struct {
    uint32_t s_inodes_count;        // Toplam inode sayısı
    uint32_t s_blocks_count;       // Toplam blok sayısı
    uint32_t s_r_blocks_count;      // Reserved blok sayısı
    uint32_t s_free_blocks_count;   // Serbest blok sayısı
    uint32_t s_free_inodes_count;   // Serbest inode sayısı
    uint32_t s_first_data_block;   // İlk veri bloğu
    uint32_t s_log_block_size;      // Log blok boyutu
    uint32_t s_log_frag_size;      // Log fragment boyutu
    uint32_t s_blocks_per_group;   // Grup başına blok sayısı
    uint32_t s_frags_per_group;    // Grup başına fragment sayısı
    uint32_t s_inodes_per_group;  // Grup başına inode sayısı
    uint32_t s_mtime;             // Mount zamanı
    uint32_t s_wtime;             // Write zamanı
    uint16_t s_mnt_count;         // Mount count
    uint16_t s_max_mnt_count;     // Max mount count
    uint16_t s_magic;             // EXT2 signature
    uint16_t s_state;             // Filesystem durumu
    uint16_t s_errors;            // Error handling
    uint16_t s_minor_rev_level;     // Minor revision level
    uint32_t s_lastcheck;         // Son kontrol zamanı
    uint32_t s_checkinterval;      // Kontrol aralığı
    uint32_t s_creator_os;         // Oluşturan OS
    uint32_t s_rev_level;          // Revision level
    uint16_t s_def_resuid;        // Default reserved user ID
    uint16_t s_def_resgid;        // Default reserved group ID
    uint32_t s_first_ino;         // İlk kullanılmayan inode
    uint16_t s_inode_size;         // Inode boyutu
    uint16_t s_block_group_nr;     // Blok grup sayısı
    uint32_t s_feature_compat;     // Compatible features
    uint32_t s_feature_incompat;   // Incompatible features
    uint32_t s_feature_ro_compat;  // Read-only compatible features
    uint8_t  s_uuid[16];         // Filesystem UUID
    uint8_t  s_volume_name[16];   // Volume name
    uint8_t  s_last_mounted[64];  // Son mount point
    uint32_t s_algorithm_usage_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;
    uint8_t  s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    uint32_t s_hash_seed[4];
    uint8_t  s_def_hash_version;
    uint8_t  s_jnl_backup_type;
    uint16_t s_desc_size;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_mkfs_time;
    uint32_t s_jnl_blocks[17];
    uint32_t s_reserved[172];
} __attribute__((packed)) ext2_superblock_t;

// EXT2 Group Descriptor
typedef struct {
    uint32_t bg_block_bitmap;      // Block bitmap blok numarası
    uint32_t bg_inode_bitmap;      // Inode bitmap blok numarası
    uint32_t bg_inode_table;      // Inode tablosu blok numarası
    uint16_t bg_free_blocks_count; // Serbest blok sayısı
    uint16_t bg_free_inodes_count; // Serbest inode sayısı
    uint16_t bg_used_dirs_count;   // Kullanılan dizin sayısı
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
} __attribute__((packed)) ext2_group_desc_t;

// EXT2 Inode
typedef struct {
    uint16_t i_mode;              // Dosya modu ve izinler
    uint16_t i_uid;               // User ID
    uint32_t i_size;              // Dosya boyutu (bytes)
    uint32_t i_atime;             // Access zamanı
    uint32_t i_ctime;             // Creation zamanı
    uint32_t i_mtime;             // Modification zamanı
    uint32_t i_dtime;             // Deletion zamanı
    uint16_t i_gid;               // Group ID
    uint16_t i_links_count;        // Link sayısı
    uint32_t i_blocks;            // Blok sayısı
    uint32_t i_flags;             // Inode flags
    uint32_t i_reserved1;
    uint32_t i_block[15];         // Direct ve indirect blok pointer'ları
    uint32_t i_generation;        // Generation number
    uint32_t i_file_acl;          // File ACL
    uint32_t i_dir_acl;           // Directory ACL
    uint32_t i_faddr;            // Fragment adresi
    uint8_t  i_frag;             // Fragment number
    uint8_t  i_fsize;            // Fragment boyutu
    uint16_t i_pad1;
    uint16_t i_uid_high;          // High 16 bits of User ID
    uint16_t i_gid_high;          // High 16 bits of Group ID
    uint32_t i_reserved2;
} __attribute__((packed)) ext2_inode_t;

// EXT2 Directory Entry
typedef struct {
    uint32_t inode;               // Inode numarası
    uint16_t rec_len;             // Record length
    uint8_t  name_len;           // Dosya adı uzunluğu
    uint8_t  file_type;           // Dosya türü (EXT2 rev1+)
    char     name[EXT2_MAX_FILENAME_LEN]; // Dosya adı
} __attribute__((packed)) ext2_dir_entry_t;

// EXT2 Filesystem Structure
typedef struct {
    ata_device_t* device;          // ATA device
    ext2_superblock_t sb;         // Superblock
    ext2_group_desc_t* groups;   // Group descriptor table
    uint8_t* block_bitmap;        // Block bitmap cache
    uint8_t* inode_bitmap;        // Inode bitmap cache
    ext2_inode_t* inode_cache;   // Inode cache
    uint32_t block_size;          // Blok boyutu
    uint32_t inodes_per_group;    // Grup başına inode sayısı
    uint32_t blocks_per_group;    // Grup başına blok sayısı
    uint32_t group_count;         // Grup sayısı
    uint32_t inode_size;         // Inode boyutu
    uint32_t mounted;            // Mount durumu
} ext2_fs_t;

// EXT2 File Handle
typedef struct {
    ext2_fs_t* fs;              // Filesystem pointer
    ext2_inode_t inode;          // Inode
    uint32_t position;           // Mevcut pozisyon
    uint32_t size;              // Dosya boyutu
    uint32_t flags;             // Açma flag'leri
} ext2_file_t;

// EXT2 Directory Iterator
typedef struct {
    ext2_fs_t* fs;              // Filesystem pointer
    ext2_inode_t dir_inode;      // Directory inode
    uint8_t* block_data;        // Blok verisi
    uint32_t block_num;          // Mevcut blok numarası
    uint32_t offset;            // Mevcut offset
    uint32_t entries_read;       // Okunan entry sayısı
} ext2_dir_iter_t;

// EXT2 Fonksiyonları
int ext2_init();
int ext2_mount(ata_device_t* device, ext2_fs_t* fs);
int ext2_unmount(ext2_fs_t* fs);
int ext2_read_superblock(ext2_fs_t* fs, ext2_superblock_t* sb);
int ext2_read_group_desc(ext2_fs_t* fs, ext2_group_desc_t* gd);
int ext2_read_inode(ext2_fs_t* fs, uint32_t inode_num, ext2_inode_t* inode);
int ext2_write_inode(ext2_fs_t* fs, uint32_t inode_num, ext2_inode_t* inode);

// Inode İşlemleri
int ext2_allocate_inode(ext2_fs_t* fs);
int ext2_free_inode(ext2_fs_t* fs, uint32_t inode_num);
int ext2_find_inode(ext2_fs_t* fs, const char* path, ext2_inode_t* inode);
int ext2_inode_to_block(ext2_fs_t* fs, ext2_inode_t* inode, uint32_t block_num);

// Blok İşlemleri
int ext2_allocate_block(ext2_fs_t* fs);
int ext2_free_block(ext2_fs_t* fs, uint32_t block_num);
int ext2_read_block(ext2_fs_t* fs, uint32_t block_num, uint8_t* buffer);
int ext2_write_block(ext2_fs_t* fs, uint32_t block_num, uint8_t* buffer);

// Directory İşlemleri
int ext2_open_dir(ext2_fs_t* fs, const char* path, ext2_dir_iter_t* iter);
int ext2_read_dir(ext2_dir_iter_t* iter, ext2_dir_entry_t* entry);
int ext2_close_dir(ext2_dir_iter_t* iter);
int ext2_find_dir_entry(ext2_fs_t* fs, ext2_inode_t* dir_inode, const char* name, ext2_dir_entry_t* entry);
int ext2_create_dir(ext2_fs_t* fs, const char* path, uint16_t mode);
int ext2_remove_dir(ext2_fs_t* fs, const char* path);

// Dosya İşlemleri
int ext2_open_file(ext2_fs_t* fs, const char* path, uint32_t flags, ext2_file_t* file);
int ext2_read_file(ext2_file_t* file, void* buffer, uint32_t size);
int ext2_write_file(ext2_file_t* file, void* buffer, uint32_t size);
int ext2_seek_file(ext2_file_t* file, uint32_t position);
int ext2_close_file(ext2_file_t* file);
int ext2_create_file(ext2_fs_t* fs, const char* path, uint16_t mode);
int ext2_delete_file(ext2_fs_t* fs, const char* path);

// İzin İşlemleri
int ext2_chmod(ext2_fs_t* fs, const char* path, uint16_t mode);
int ext2_get_permissions(ext2_fs_t* fs, const char* path, uint16_t* mode);
int ext2_check_permissions(ext2_inode_t* inode, uint16_t required_perm, uint32_t uid, uint32_t gid);

// Utility Fonksiyonları
const char* ext2_get_file_type_name(uint8_t file_type);
void ext2_print_permissions(uint16_t mode);
void ext2_print_inode_info(ext2_inode_t* inode);
const char* ext2_format_size(uint32_t size);
int ext2_is_regular_file(ext2_inode_t* inode);
int ext2_is_directory(ext2_inode_t* inode);
int ext2_is_symlink(ext2_inode_t* inode);

// Global EXT2 filesystem
extern ext2_fs_t ext2_main_fs;

#endif
