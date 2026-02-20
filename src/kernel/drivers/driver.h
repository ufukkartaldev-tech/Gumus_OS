#ifndef DRIVER_H
#define DRIVER_H

#include <stdint.h>
#include <stddef.h>

// Sürücü Tipleri
typedef enum {
    DRIVER_TYPE_CHAR,
    DRIVER_TYPE_BLOCK,
    DRIVER_TYPE_NET,
    DRIVER_TYPE_DISPLAY,
    DRIVER_TYPE_INPUT
} driver_type_t;

// Sürücü Arayüzü (Interface)
typedef struct driver {
    char name[32];
    driver_type_t type;
    
    // Fonksiyon İşaretçileri (Virtual Functions)
    int (*init)(void);
    int (*read)(void* buffer, uint32_t size, uint32_t offset);
    int (*write)(void* buffer, uint32_t size, uint32_t offset);
    int (*ioctl)(uint32_t command, void* arg);
    int (*shutdown)(void);
} driver_t;

// Sürücü Yöneticisi Fonksiyonları
void driver_manager_init();
void driver_register(driver_t* driver);
driver_t* driver_get(const char* name);

#endif
