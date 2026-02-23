#ifndef FS_H
#define FS_H

#include <stdint.h>

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t file_size;
} __attribute__((packed)) fat_directory_entry_t;

void fs_init();
void fs_ls();
void fs_cat(const char* filename);
int fs_write(const char* filename, const char* data);
int fs_write_bin(const char* filename, const uint8_t* data, uint32_t size);
char* fs_read(const char* filename);
uint8_t* fs_read_bin(const char* filename, uint32_t* out_size);

#endif
