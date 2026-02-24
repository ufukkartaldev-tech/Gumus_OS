#include "fs.h"
#include "ata.h"
#include "memory.h"
#include "kernel.h"
#include "string.h"

// Root directory sector (GÃ¼mÃ¼ÅŸOS iÃ§in geÃ§ici olarak sabit bir sektÃ¶r seÃ§iyoruz)
#define ROOT_DIR_SECTOR 100

void fs_init() {
    // Gelecekte BPB okunacak
}

void fs_ls() {
    uint8_t* buffer = kmalloc(512);
    ata_read_sectors((uint32_t)buffer, ROOT_DIR_SECTOR, 1);

    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;

    print("\nDosya Listesi:\n");
    print("Isim        Boyut\n");
    print("----------  -----\n");

    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
        if (entry[i].name[0] == 0) break; // BoÅŸ giriÅŸ
        if (entry[i].name[0] == 0xE5) continue; // SilinmiÅŸ giriÅŸ

        // Ä°sim basÄ±mÄ±
        char name[13];
        int k = 0;
        for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) name[k++] = entry[i].name[j];
        if (entry[i].ext[0] != ' ') {
            name[k++] = '.';
            for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) name[k++] = entry[i].ext[j];
        }
        name[k] = '\0';

        print(name);
        for (int j = strlen(name); j < 12; j++) print(" ");
        
        char size_buf[16];
        itoa(entry[i].file_size, size_buf);
        print(size_buf);
        print(" byte\n");
    }

    kfree(buffer);
}

void fs_cat(const char* filename) {
    uint8_t* buffer = kmalloc(512);
    ata_read_sectors((uint32_t)buffer, ROOT_DIR_SECTOR, 1);

    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;
    int found = 0;

    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
        if (entry[i].name[0] == 0) break;
        
        char name[13];
        int k = 0;
        for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) name[k++] = entry[i].name[j];
        if (entry[i].ext[0] != ' ') {
            name[k++] = '.';
            for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) name[k++] = entry[i].ext[j];
        }
        name[k] = '\0';

        if (strcasecmp(name, filename) == 0) {
            found = 1;
            uint32_t cluster = entry[i].first_cluster_lo;
            // Basitlik iÃ§in cluster = sektÃ¶r varsayÄ±yoruz (Pseudo-FAT)
            uint8_t* data = kmalloc(512);
            ata_read_sectors((uint32_t)data, 200 + cluster, 1);
            print("\n--- "); print(name); print(" ---\n");
            print((char*)data);
            print("\n----------------\n");
            kfree(data);
            break;
        }
    }

    if (!found) {
        print("\nHata: Dosya bulunamadi.\n");
    }
    kfree(buffer);
}

int fs_write_bin(const char* filename, const uint8_t* data, uint32_t size) {
    uint8_t* buffer = kmalloc(512);
    ata_read_sectors((uint32_t)buffer, ROOT_DIR_SECTOR, 1);
    fat_directory_entry_t* root = (fat_directory_entry_t*)buffer;

    int free_entry = -1;
    uint32_t max_end_cluster = 0;

    // BoÅŸ yer bul ve kullanÄ±lan son cluster'Ä± tespit et
    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
        if (root[i].name[0] == 0 || root[i].name[0] == 0xE5) {
            if (free_entry == -1) free_entry = i;
        } else {
            uint32_t start = root[i].first_cluster_lo;
            uint32_t secs = (root[i].file_size + 511) / 512;
            if (secs == 0) secs = 1; // En az 1 sektÃ¶r varsay
            if (start + secs > max_end_cluster) {
                max_end_cluster = start + secs;
            }
        }
    }

    if (free_entry == -1) {
        kfree(buffer);
        return 0; // Dolu
    }

    // Ä°sim AyrÄ±ÅŸtÄ±rma
    char name[8]; memset(name, ' ', 8);
    char ext[3]; memset(ext, ' ', 3);
    
    // Basit parse
    int i = 0, k = 0;
    while(filename[i] != '.' && filename[i] != 0 && k < 8) name[k++] = filename[i++];
    if (filename[i] == '.') {
        i++; k = 0;
        while(filename[i] != 0 && k < 3) ext[k++] = filename[i++];
    }
    
    // GiriÅŸi Yaz
    memcpy(root[free_entry].name, name, 8);
    memcpy(root[free_entry].ext, ext, 3);
    root[free_entry].file_size = size;
    root[free_entry].first_cluster_lo = max_end_cluster + 1; // Yeni baÅŸlangÄ±Ã§
    root[free_entry].attributes = 0x20;

    // Dizin GÃ¼ncelle
    ata_write_sectors(ROOT_DIR_SECTOR, 1, (uint32_t)buffer);
    
    // Veriyi Yaz
    uint32_t sector_count = (size + 511) / 512;
    uint32_t start_sector = 200 + root[free_entry].first_cluster_lo;
    
    // Tek seferde veya parÃ§a parÃ§a yazabiliriz
    // ata_write_sectors ardÄ±ÅŸÄ±k yazmayÄ± destekliyor
    // Ancak data buffer linear olmalÄ±.
    // paint->canvas linear mi? Evet (kmalloc).
    // O zaman tek seferde yazabiliriz.
    
    ata_write_sectors(start_sector, sector_count, (uint32_t)data);

    kfree(buffer);
    return 1;
}

int fs_write(const char* filename, const char* data) {
    return fs_write_bin(filename, (const uint8_t*)data, strlen(data));
}

char* fs_read(const char* filename) {
    // Legacy support (512 byte)
    uint32_t size;
    return (char*)fs_read_bin(filename, &size);
}

uint8_t* fs_read_bin(const char* filename, uint32_t* out_size) {
    uint8_t* buffer = kmalloc(512);
    ata_read_sectors((uint32_t)buffer, ROOT_DIR_SECTOR, 1);

    fat_directory_entry_t* entry = (fat_directory_entry_t*)buffer;

    for (int i = 0; i < 512 / sizeof(fat_directory_entry_t); i++) {
        if (entry[i].name[0] == 0) break;
        
        char name[13];
        int k = 0;
        for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) name[k++] = entry[i].name[j];
        if (entry[i].ext[0] != ' ') {
            name[k++] = '.';
            for (int j = 0; j < 3 && entry[i].ext[j] != ' '; j++) name[k++] = entry[i].ext[j];
        }
        name[k] = '\0';

        if (strcasecmp(name, filename) == 0) {
            uint32_t cluster = entry[i].first_cluster_lo;
            uint32_t size = entry[i].file_size;
            if (out_size) *out_size = size;
            
            // TÃ¼m dosyayÄ± oku
            uint32_t sector_count = (size + 511) / 512;
            uint8_t* data = kmalloc(sector_count * 512 + 1); // +1 safety null terminator
            
            ata_read_sectors((uint32_t)data, 200 + cluster, sector_count);
            data[size] = '\0'; // DayÄ± Tavsiyesi: String korumasÄ±
            
            kfree(buffer);
            return data;
        }
    }

    kfree(buffer);
    return (void*)0;
}
