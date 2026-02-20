#include "driver.h"
#include "string.h"
#include "hardware_detect.h"

#define MAX_DRIVERS 32

static driver_t* drivers[MAX_DRIVERS];
static int driver_count = 0;
static driver_t* active_drivers[MAX_DRIVERS];
static int active_driver_count = 0;

void driver_manager_init() {
    for(int i=0; i<MAX_DRIVERS; i++) {
        drivers[i] = 0;
        active_drivers[i] = 0;
    }
    driver_count = 0;
    active_driver_count = 0;
}

void driver_register(driver_t* driver) {
    if (driver_count < MAX_DRIVERS) {
        drivers[driver_count++] = driver;
        printf("Sürücü kaydedildi: %s\n", driver->name);
    }
}

int driver_activate(const char* name) {
    driver_t* driver = driver_get(name);
    if (!driver) return -1;
    
    // Sürücüyü başlat
    if (driver->init && driver->init() == 0) {
        if (active_driver_count < MAX_DRIVERS) {
            active_drivers[active_driver_count++] = driver;
            printf("Sürücü aktifleştirildi: %s\n", name);
            return 0;
        }
    }
    return -1;
}

int driver_deactivate(const char* name) {
    for(int i=0; i<active_driver_count; i++) {
        if (strcmp(active_drivers[i]->name, name) == 0) {
            // Sürücüyü kapat
            if (active_drivers[i]->shutdown) {
                active_drivers[i]->shutdown();
            }
            
            // Listeden çıkar
            for(int j=i; j<active_driver_count-1; j++) {
                active_drivers[j] = active_drivers[j+1];
            }
            active_driver_count--;
            printf("Sürücü devre dışı bırakıldı: %s\n", name);
            return 0;
        }
    }
    return -1;
}

int driver_unregister(const char* name) {
    // Önce devre dışı bırak
    driver_deactivate(name);
    
    // Kayıttan sil
    for(int i=0; i<driver_count; i++) {
        if (strcmp(drivers[i]->name, name) == 0) {
            for(int j=i; j<driver_count-1; j++) {
                drivers[j] = drivers[j+1];
            }
            driver_count--;
            printf("Sürücü kayıttan silindi: %s\n", name);
            return 0;
        }
    }
    return -1;
}

driver_t* driver_get(const char* name) {
    for(int i=0; i<driver_count; i++) {
        if (strcmp(drivers[i]->name, name) == 0) {
            return drivers[i];
        }
    }
    return 0;
}

void driver_list_all() {
    printf("\n=== Kayıtlı Sürüciler ===\n");
    for(int i=0; i<driver_count; i++) {
        printf("%d. %s (Tip: %d)\n", i+1, drivers[i]->name, drivers[i]->type);
    }
    printf("========================\n");
}

void driver_list_active() {
    printf("\n=== Aktif Sürüciler ===\n");
    for(int i=0; i<active_driver_count; i++) {
        printf("%d. %s (Tip: %d)\n", i+1, active_drivers[i]->name, active_drivers[i]->type);
    }
    printf("=======================\n");
}

int driver_auto_load_all() {
    hardware_info_t* hw_info = hardware_get_info();
    int loaded_count = 0;
    
    printf("Otomatik sürücü yüklemesi başlatılıyor...\n");
    
    for(int i=0; i<hw_info->pci_device_count; i++) {
        if (hardware_load_driver_for_device(&hw_info->pci_devices[i]) == 0) {
            loaded_count++;
        }
    }
    
    printf("Toplam %d sürücü yüklendi.\n", loaded_count);
    return loaded_count;
}
