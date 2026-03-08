#include "ext2.h"
#include "ata.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "printf.h"

// Global EXT2 filesystem
ext2_fs_t ext2_main_fs;

// EXT2'yi başlat
int ext2_init() {
    printf("EXT2 Filesystem başlatılıyor...\n");
    
    // Main filesystem'i sıfırla
    memset(&ext2_main_fs, 0, sizeof(ext2_fs_t));
    ext2_main_fs.mounted = 0;
    
    printf("EXT2 Filesystem hazır.\n");
    return 0;
}

// EXT2 filesystem mount et
int ext2_mount(ata_device_t* device, ext2_fs_t* fs) {
    if (!device || !fs) {
        return -1;
    }
    
    printf("EXT2 filesystem mount ediliyor...\n");
    
    fs->device = device;
    
    // Superblock'ı oku
    if (ext2_read_superblock(fs, &fs->sb) < 0) {
        printf("EXT2 superblock okunamadı.\n");
        return -2;
    }
    
    // EXT2 signature kontrolü
    if (fs->sb.s_magic != EXT2_SIGNATURE) {
        printf("Geçersiz EXT2 signature: 0x%04X\n", fs->sb.s_magic);
        return -3;
    }
    
    // Filesystem parametrelerini ayarla
    fs->block_size = EXT2_BLOCK_SIZE << fs->sb.s_log_block_size;
    fs->inodes_per_group = fs->sb.s_inodes_per_group;
    fs->blocks_per_group = fs->sb.s_blocks_per_group;
    fs->inode_size = fs->sb.s_inode_size;
    fs->group_count = (fs->sb.s_blocks_count + fs->blocks_per_group - 1) / fs->blocks_per_group;
    
    printf("EXT2 v%d.%d, Block size: %d, Inode size: %d\n",
           fs->sb.s_rev_level, fs->sb.s_minor_rev_level,
           fs->block_size, fs->inode_size);
    
    // Group descriptor table'ı oku
    if (ext2_read_group_desc(fs, fs->groups) < 0) {
        printf("Group descriptor table okunamadı.\n");
        return -4;
    }
    
    fs->mounted = 1;
    printf("EXT2 filesystem başarıyla mount edildi.\n");
    return 0;
}

// EXT2 filesystem unmount et
int ext2_unmount(ext2_fs_t* fs) {
    if (!fs || !fs->mounted) {
        return -1;
    }
    
    printf("EXT2 filesystem unmount ediliyor...\n");
    
    // Cache'leri serbest bırak
    if (fs->block_bitmap) {
        free(fs->block_bitmap);
        fs->block_bitmap = NULL;
    }
    
    if (fs->inode_bitmap) {
        free(fs->inode_bitmap);
        fs->inode_bitmap = NULL;
    }
    
    if (fs->inode_cache) {
        free(fs->inode_cache);
        fs->inode_cache = NULL;
    }
    
    if (fs->groups) {
        free(fs->groups);
        fs->groups = NULL;
    }
    
    fs->mounted = 0;
    printf("EXT2 filesystem unmount edildi.\n");
    return 0;
}

// EXT2 superblock'ı oku
int ext2_read_superblock(ext2_fs_t* fs, ext2_superblock_t* sb) {
    if (!fs || !sb) {
        return -1;
    }
    
    // Superblock'ı diskten oku
    uint8_t buffer[EXT2_BLOCK_SIZE];
    if (ata_read(fs->device, EXT2_SUPERBLOCK_OFFSET / EXT2_SECTOR_SIZE, 
                 buffer, EXT2_BLOCK_SIZE / EXT2_SECTOR_SIZE) < 0) {
        printf("Superblock okuma hatası.\n");
        return -2;
    }
    
    memcpy(sb, buffer, sizeof(ext2_superblock_t));
    return 0;
}

// Group descriptor table'ı oku
int ext2_read_group_desc(ext2_fs_t* fs, ext2_group_desc_t* gd) {
    if (!fs || !gd) {
        return -1;
    }
    
    // Group descriptor table'ı bellekte ayır
    uint32_t gd_size = fs->group_count * sizeof(ext2_group_desc_t);
    gd = (ext2_group_desc_t*)malloc(gd_size);
    if (!gd) {
        printf("Group descriptor table için bellek ayrılamadı.\n");
        return -2;
    }
    
    // Group descriptor table'ı diskten oku
    uint32_t gd_block = 2; // Genellikle blok 2'de başlar
    uint8_t buffer[EXT2_BLOCK_SIZE];
    
    if (ata_read(fs->device, (gd_block * fs->block_size) / EXT2_SECTOR_SIZE,
                 buffer, EXT2_BLOCK_SIZE / EXT2_SECTOR_SIZE) < 0) {
        printf("Group descriptor table okuma hatası.\n");
        free(gd);
        return -3;
    }
    
    memcpy(gd, buffer, gd_size);
    fs->groups = gd;
    
    return 0;
}

// Inode oku
int ext2_read_inode(ext2_fs_t* fs, uint32_t inode_num, ext2_inode_t* inode) {
    if (!fs || !inode || inode_num == 0) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    // Inode numarasını group ve index'e çevir
    inode_num--; // Inode numaraları 1'den başlar
    uint32_t group = inode_num / fs->inodes_per_group;
    uint32_t index = inode_num % fs->inodes_per_group;
    
    if (group >= fs->group_count) {
        printf("Geçersiz inode grubu: %d\n", group);
        return -3;
    }
    
    // Inode tablosu blok numarasını hesapla
    uint32_t inode_table_block = fs->groups[group].bg_inode_table;
    uint32_t inode_offset = index * fs->inode_size;
    uint32_t block_offset = inode_offset / fs->block_size;
    uint32_t byte_offset = inode_offset % fs->block_size;
    
    // Inode bloğunu oku
    uint8_t buffer[EXT2_BLOCK_SIZE];
    uint32_t block_to_read = inode_table_block + block_offset;
    uint32_t sector = (block_to_read * fs->block_size) / EXT2_SECTOR_SIZE;
    
    if (ata_read(fs->device, sector, buffer, fs->block_size / EXT2_SECTOR_SIZE) < 0) {
        printf("Inode okuma hatası.\n");
        return -4;
    }
    
    // Inode'u kopyala
    memcpy(inode, buffer + byte_offset, sizeof(ext2_inode_t));
    
    return 0;
}

// Inode yaz
int ext2_write_inode(ext2_fs_t* fs, uint32_t inode_num, ext2_inode_t* inode) {
    if (!fs || !inode || inode_num == 0) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    // Inode numarasını group ve index'e çevir
    inode_num--;
    uint32_t group = inode_num / fs->inodes_per_group;
    uint32_t index = inode_num % fs->inodes_per_group;
    
    // Inode tablosu blok numarasını hesapla
    uint32_t inode_table_block = fs->groups[group].bg_inode_table;
    uint32_t inode_offset = index * fs->inode_size;
    uint32_t block_offset = inode_offset / fs->block_size;
    uint32_t byte_offset = inode_offset % fs->block_size;
    
    // Inode bloğunu oku
    uint8_t buffer[EXT2_BLOCK_SIZE];
    uint32_t block_to_read = inode_table_block + block_offset;
    uint32_t sector = (block_to_read * fs->block_size) / EXT2_SECTOR_SIZE;
    
    if (ata_read(fs->device, sector, buffer, fs->block_size / EXT2_SECTOR_SIZE) < 0) {
        printf("Inode bloğu okuma hatası.\n");
        return -3;
    }
    
    // Inode'u güncelle
    memcpy(buffer + byte_offset, inode, sizeof(ext2_inode_t));
    
    // Inode bloğunu geri yaz
    if (ata_write(fs->device, sector, buffer, fs->block_size / EXT2_SECTOR_SIZE) < 0) {
        printf("Inode yazma hatası.\n");
        return -4;
    }
    
    return 0;
}

// Blok oku
int ext2_read_block(ext2_fs_t* fs, uint32_t block_num, uint8_t* buffer) {
    if (!fs || !buffer) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    uint32_t sector = (block_num * fs->block_size) / EXT2_SECTOR_SIZE;
    uint32_t sectors = fs->block_size / EXT2_SECTOR_SIZE;
    
    if (ata_read(fs->device, sector, buffer, sectors) < 0) {
        printf("Blok okuma hatası: %d\n", block_num);
        return -3;
    }
    
    return 0;
}

// Blok yaz
int ext2_write_block(ext2_fs_t* fs, uint32_t block_num, uint8_t* buffer) {
    if (!fs || !buffer) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    uint32_t sector = (block_num * fs->block_size) / EXT2_SECTOR_SIZE;
    uint32_t sectors = fs->block_size / EXT2_SECTOR_SIZE;
    
    if (ata_write(fs->device, sector, buffer, sectors) < 0) {
        printf("Blok yazma hatası: %d\n", block_num);
        return -3;
    }
    
    return 0;
}

// Dosya türü adını al
const char* ext2_get_file_type_name(uint8_t file_type) {
    switch (file_type) {
        case EXT2_INODE_REGULAR:   return "dosya";
        case EXT2_INODE_DIRECTORY: return "dizin";
        case EXT2_INODE_SYMLINK:   return "link";
        case EXT2_INODE_CHARDEV:   return "chr";
        case EXT2_INODE_BLOCKDEV:  return "blk";
        case EXT2_INODE_FIFO:      return "fifo";
        case EXT2_INODE_SOCK:      return "sock";
        default:                  return "bilinmeyen";
    }
}

// İzinleri formatla ve yazdır
void ext2_print_permissions(uint16_t mode) {
    // Dosya türü
    if (S_ISDIR(mode)) {
        printf("d");
    } else if (S_ISLNK(mode)) {
        printf("l");
    } else if (S_ISBLK(mode)) {
        printf("b");
    } else if (S_ISCHR(mode)) {
        printf("c");
    } else if (S_ISFIFO(mode)) {
        printf("p");
    } else if (S_ISSOCK(mode)) {
        printf("s");
    } else {
        printf("-");
    }
    
    // Owner izinleri
    printf((mode & EXT2_S_IRUSR) ? "r" : "-");
    printf((mode & EXT2_S_IWUSR) ? "w" : "-");
    printf((mode & EXT2_S_IXUSR) ? "x" : "-");
    
    // Group izinleri
    printf((mode & EXT2_S_IRGRP) ? "r" : "-");
    printf((mode & EXT2_S_IWGRP) ? "w" : "-");
    printf((mode & EXT2_S_IXGRP) ? "x" : "-");
    
    // Others izinleri
    printf((mode & EXT2_S_IROTH) ? "r" : "-");
    printf((mode & EXT2_S_IWOTH) ? "w" : "-");
    printf((mode & EXT2_S_IXOTH) ? "x" : "-");
    
    // Special bits
    if (mode & EXT2_S_ISUID) printf(" (suid)");
    if (mode & EXT2_S_ISGID) printf(" (sgid)");
    if (mode & EXT2_S_ISVTX) printf(" (sticky)");
}

// Inode bilgisini yazdır
void ext2_print_inode_info(ext2_inode_t* inode) {
    printf("=== Inode Bilgisi ===\n");
    printf("Mode: 0%o ", inode->i_mode);
    ext2_print_permissions(inode->i_mode);
    printf("\n");
    printf("UID: %d, GID: %d\n", inode->i_uid, inode->i_gid);
    printf("Boyut: %u bytes\n", inode->i_size);
    printf("Link sayısı: %d\n", inode->i_links_count);
    printf("Blok sayısı: %d\n", inode->i_blocks);
    printf("Oluşturma: %s", ctime(&inode->i_ctime));
    printf("Değiştirme: %s", ctime(&inode->i_mtime));
    printf("Erişim: %s", ctime(&inode->i_atime));
    printf("=====================\n");
}

// Boyutu formatla
const char* ext2_format_size(uint32_t size) {
    static char buffer[32];
    
    if (size < 1024) {
        sprintf(buffer, "%u B", size);
    } else if (size < 1024 * 1024) {
        sprintf(buffer, "%.1f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        sprintf(buffer, "%.1f MB", size / (1024.0 * 1024.0));
    } else {
        sprintf(buffer, "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
    
    return buffer;
}

// Regular dosya mı?
int ext2_is_regular_file(ext2_inode_t* inode) {
    return ((inode->i_mode & EXT2_S_IFMT) == EXT2_S_IFREG);
}

// Dizin mi?
int ext2_is_directory(ext2_inode_t* inode) {
    return ((inode->i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR);
}

// Sembolik link mi?
int ext2_is_symlink(ext2_inode_t* inode) {
    return ((inode->i_mode & EXT2_S_IFMT) == EXT2_S_IFLNK);
}

// İzinleri kontrol et
int ext2_check_permissions(ext2_inode_t* inode, uint16_t required_perm, uint32_t uid, uint32_t gid) {
    uint16_t mode = inode->i_mode;
    
    // Owner ise
    if (uid == inode->i_uid) {
        return (mode & (required_perm << 6)) != 0;
    }
    
    // Group üyesi ise
    if (gid == inode->i_gid) {
        return (mode & (required_perm << 3)) != 0;
    }
    
    // Others ise
    return (mode & required_perm) != 0;
}

// Directory aç
int ext2_open_dir(ext2_fs_t* fs, const char* path, ext2_dir_iter_t* iter) {
    if (!fs || !path || !iter) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    // Path'in inode'unu bul
    ext2_inode_t dir_inode;
    if (ext2_find_inode(fs, path, &dir_inode) < 0) {
        printf("Dizin bulunamadı: %s\n", path);
        return -3;
    }
    
    if (!ext2_is_directory(&dir_inode)) {
        printf("Bir dizin değil: %s\n", path);
        return -4;
    }
    
    // Iterator'ı başlat
    iter->fs = fs;
    iter->dir_inode = dir_inode;
    iter->block_data = NULL;
    iter->block_num = 0;
    iter->offset = 0;
    iter->entries_read = 0;
    
    return 0;
}

// Directory oku
int ext2_read_dir(ext2_dir_iter_t* iter, ext2_dir_entry_t* entry) {
    if (!iter || !entry) {
        return -1;
    }
    
    // İlk bloğu oku
    if (iter->block_num == 0) {
        iter->block_data = malloc(iter->fs->block_size);
        if (!iter->block_data) {
            printf("Directory bloğu için bellek ayrılamadı.\n");
            return -2;
        }
        
        // Direkt bloğu oku
        uint32_t block_num = iter->dir_inode.i_block[0];
        if (ext2_read_block(iter->fs, block_num, iter->block_data) < 0) {
            printf("Directory bloğu okunamadı.\n");
            return -3;
        }
    }
    
    // Directory entry'leri tara
    uint8_t* ptr = iter->block_data + iter->offset;
    ext2_dir_entry_t* dir_entry = (ext2_dir_entry_t*)ptr;
    
    // Entry'nin sonunu kontrol et
    if (dir_entry->inode == 0 || iter->offset >= iter->fs->block_size) {
        return -4; // Son entry
    }
    
    // Entry'yi kopyala
    entry->inode = dir_entry->inode;
    entry->rec_len = dir_entry->rec_len;
    entry->name_len = dir_entry->name_len;
    entry->file_type = dir_entry->file_type;
    memcpy(entry->name, dir_entry->name, dir_entry->name_len);
    entry->name[dir_entry->name_len] = '\0';
    
    // Offset'i güncelle
    iter->offset += dir_entry->rec_len;
    iter->entries_read++;
    
    return 0;
}

// Directory kapat
int ext2_close_dir(ext2_dir_iter_t* iter) {
    if (!iter) {
        return -1;
    }
    
    if (iter->block_data) {
        free(iter->block_data);
        iter->block_data = NULL;
    }
    
    return 0;
}

// Path'ten inode bul
int ext2_find_inode(ext2_fs_t* fs, const char* path, ext2_inode_t* inode) {
    if (!fs || !path || !inode) {
        return -1;
    }
    
    if (!fs->mounted) {
        printf("EXT2 filesystem mount edilmemiş.\n");
        return -2;
    }
    
    // Root dizininden başla
    ext2_inode_t current_inode;
    if (ext2_read_inode(fs, EXT2_ROOT_INODE, &current_inode) < 0) {
        printf("Root inode okunamadı.\n");
        return -3;
    }
    
    // Path'i parse et
    char path_copy[EXT2_MAX_PATH_LEN];
    strcpy(path_copy, path);
    
    char* token = strtok(path_copy, "/");
    while (token != NULL) {
        if (!ext2_is_directory(&current_inode)) {
            printf("Bir dizin değil.\n");
            return -4;
        }
        
        // Directory'de entry ara
        ext2_dir_iter_t iter;
        if (ext2_open_dir(fs, token, &iter) < 0) {
            return -5;
        }
        
        ext2_dir_entry_t entry;
        int found = 0;
        
        while (ext2_read_dir(&iter, &entry) == 0) {
            if (strcmp(entry.name, token) == 0) {
                if (ext2_read_inode(fs, entry.inode, &current_inode) < 0) {
                    ext2_close_dir(&iter);
                    return -6;
                }
                found = 1;
                break;
            }
        }
        
        ext2_close_dir(&iter);
        
        if (!found) {
            printf("Dosya/dizin bulunamadı: %s\n", token);
            return -7;
        }
        
        token = strtok(NULL, "/");
    }
    
    // Son inode'u kopyala
    memcpy(inode, &current_inode, sizeof(ext2_inode_t));
    return 0;
}

// Dosya aç
int ext2_open_file(ext2_fs_t* fs, const char* path, uint32_t flags, ext2_file_t* file) {
    if (!fs || !path || !file) {
        return -1;
    }
    
    // Inode'u bul
    ext2_inode_t inode;
    if (ext2_find_inode(fs, path, &inode) < 0) {
        printf("Dosya bulunamadı: %s\n", path);
        return -2;
    }
    
    if (!ext2_is_regular_file(&inode)) {
        printf("Regular dosya değil: %s\n", path);
        return -3;
    }
    
    // File handle'ı başlat
    file->fs = fs;
    file->inode = inode;
    file->position = 0;
    file->size = inode.i_size;
    file->flags = flags;
    
    return 0;
}

// Dosyadan oku
int ext2_read_file(ext2_file_t* file, void* buffer, uint32_t size) {
    if (!file || !buffer) {
        return -1;
    }
    
    if (file->position >= file->size) {
        return 0; // EOF
    }
    
    uint32_t bytes_to_read = size;
    if (file->position + bytes_to_read > file->size) {
        bytes_to_read = file->size - file->position;
    }
    
    uint32_t bytes_read = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    
    while (bytes_read < bytes_to_read) {
        // Mevcut blok numarasını hesapla
        uint32_t block_num = file->position / file->fs->block_size;
        uint32_t block_offset = file->position % file->fs->block_size;
        uint32_t bytes_in_block = file->fs->block_size - block_offset;
        
        if (bytes_in_block > bytes_to_read - bytes_read) {
            bytes_in_block = bytes_to_read - bytes_read;
        }
        
        // Bloğu oku
        uint8_t block_buffer[file->fs->block_size];
        if (ext2_read_block(file->fs, file->inode.i_block[block_num], block_buffer) < 0) {
            printf("Dosya bloğu okunamadı.\n");
            return -2;
        }
        
        // Veriyi kopyala
        memcpy(buf_ptr + bytes_read, block_buffer + block_offset, bytes_in_block);
        
        bytes_read += bytes_in_block;
        file->position += bytes_in_block;
    }
    
    return bytes_read;
}

// Dosyaya yaz
int ext2_write_file(ext2_file_t* file, void* buffer, uint32_t size) {
    if (!file || !buffer) {
        return -1;
    }
    
    // Şimdilik sadece mevcut dosya boyutuna kadar yaz
    if (file->position + size > file->size) {
        size = file->size - file->position;
    }
    
    uint32_t bytes_written = 0;
    uint8_t* buf_ptr = (uint8_t*)buffer;
    
    while (bytes_written < size) {
        // Mevcut blok numarasını hesapla
        uint32_t block_num = file->position / file->fs->block_size;
        uint32_t block_offset = file->position % file->fs->block_size;
        uint32_t bytes_in_block = file->fs->block_size - block_offset;
        
        if (bytes_in_block > size - bytes_written) {
            bytes_in_block = size - bytes_written;
        }
        
        // Bloğu oku
        uint8_t block_buffer[file->fs->block_size];
        if (ext2_read_block(file->fs, file->inode.i_block[block_num], block_buffer) < 0) {
            printf("Dosya bloğu okunamadı.\n");
            return -2;
        }
        
        // Veriyi güncelle
        memcpy(block_buffer + block_offset, buf_ptr + bytes_written, bytes_in_block);
        
        // Bloğu geri yaz
        if (ext2_write_block(file->fs, file->inode.i_block[block_num], block_buffer) < 0) {
            printf("Dosya bloğu yazılamadı.\n");
            return -3;
        }
        
        bytes_written += bytes_in_block;
        file->position += bytes_in_block;
    }
    
    return bytes_written;
}

// Dosyayı kapat
int ext2_close_file(ext2_file_t* file) {
    if (!file) {
        return -1;
    }
    
    // Inode'u güncelle (değişiklik zamanı)
    // file->inode.i_mtime = current_time();
    // ext2_write_inode(file->fs, /* inode_num */, &file->inode);
    
    return 0;
}
