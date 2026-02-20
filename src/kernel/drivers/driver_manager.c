#include "driver.h"
#include "string.h"
#include "hardware_detect.h"
#include "../core/memory.h"

// Global sürücü yöneticisi
static driver_manager_t driver_mgr;

// Yardımcı fonksiyonlar
static driver_node_t* create_driver_node(driver_t* driver) {
    driver_node_t* node = malloc(sizeof(driver_node_t));
    if (!node) {
        printf("KRİTİK HATA: Bellek yetersiz - driver_node oluşturulamadı!\n");
        printf("Sistem panik moduna geçiyor...\n");
        // TODO: Implement kernel panic
        while(1) { asm volatile("hlt"); } // Kernel panic
    }
    node->driver = driver;
    node->next = NULL;
    return node;
}

static void free_driver_node(driver_node_t* node) {
    if (node) {
        free(node);
    }
}

static void add_driver_to_list(driver_node_t** list, driver_t* driver) {
    driver_node_t* new_node = create_driver_node(driver);
    new_node->next = *list;
    *list = new_node;
}

static int remove_driver_from_list(driver_node_t** list, driver_t* driver) {
    driver_node_t** current = list;
    
    while (*current) {
        if ((*current)->driver == driver) {
            driver_node_t* to_remove = *current;
            *current = (*current)->next;
            free_driver_node(to_remove);
            return 0;
        }
        current = &(*current)->next;
    }
    return -1;
}

static void free_driver_list(driver_node_t** list) {
    driver_node_t* current = *list;
    while (current) {
        driver_node_t* next = current->next;
        free_driver_node(current);
        current = next;
    }
    *list = NULL;
}

static int find_in_list(driver_node_t* list, driver_t* driver) {
    driver_node_t* current = list;
    while (current) {
        if (current->driver == driver) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void driver_manager_cleanup() {
    printf("Sürücü yöneticisi temizleniyor...\n");
    
    // Önce tüm aktif sürücüleri kapat
    driver_node_t* current = driver_mgr.active_list;
    while (current) {
        if (current->driver && current->driver->shutdown) {
            current->driver->shutdown();
        }
        current = current->next;
    }
    
    // Aktif listeyi temizle
    free_driver_list(&driver_mgr.active_list);
    driver_mgr.active_count = 0;
    
    // Kayıtlı listeyi temizle
    free_driver_list(&driver_mgr.driver_list);
    driver_mgr.driver_count = 0;
    
    printf("Sürücü yöneticisi temizlendi\n");
}

void driver_manager_init() {
    driver_mgr.driver_list = NULL;
    driver_mgr.active_list = NULL;
    driver_mgr.driver_count = 0;
    driver_mgr.active_count = 0;
    driver_mgr.next_unique_id = 1;
    
    printf("Sürücü yöneticisi başlatıldı\n");
}

void driver_register(driver_t* driver) {
    if (!driver) {
        printf("HATA: Boş sürücü kaydedilemez\n");
        return;
    }
    
    // Unique ID ata
    driver->unique_id = driver_mgr.next_unique_id++;
    
    // Listeye ekle
    add_driver_to_list(&driver_mgr.driver_list, driver);
    driver_mgr.driver_count++;
    
    printf("Sürücü kaydedildi: %s (ID: %d, %04X:%04X)\n", 
           driver->name, driver->unique_id, driver->vendor_id, driver->device_id);
}

int driver_activate(const char* name) {
    if (!name || !*name) {
        printf("driver_activate: Geçersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_activate: Sürücü bulunamadı: %s\n", name);
        return -1;
    }
    
    // Zaten aktif mi kontrol et
    if (find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_activate: Sürücü zaten aktif: %s\n", name);
        return 0;
    }
    
    // Sürücüyü başlat
    if (driver->init && driver->init() == 0) {
        // Aktif listeye ekle
        add_driver_to_list(&driver_mgr.active_list, driver);
        driver_mgr.active_count++;
        printf("Sürücü aktifleştirildi: %s\n", name);
        return 0;
    } else {
        printf("driver_activate: Sürücü başlatılamadı: %s\n", name);
        return -1;
    }
}

int driver_activate_by_id(uint32_t unique_id) {
    driver_t* driver = driver_get_by_unique_id(unique_id);
    if (!driver) {
        printf("driver_activate_by_id: Sürücü bulunamadı: ID %d\n", unique_id);
        return -1;
    }
    
    // Zaten aktif mi kontrol et
    if (find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_activate_by_id: Sürücü zaten aktif: ID %d\n", unique_id);
        return 0;
    }
    
    // Sürücüyü başlat
    if (driver->init && driver->init() == 0) {
        // Aktif listeye ekle
        add_driver_to_list(&driver_mgr.active_list, driver);
        driver_mgr.active_count++;
        printf("Sürücü aktifleştirildi: %s (ID: %d)\n", driver->name, unique_id);
        return 0;
    } else {
        printf("driver_activate_by_id: Sürücü başlatılamadı: ID %d\n", unique_id);
        return -1;
    }
}

int driver_deactivate(const char* name) {
    if (!name || !*name) {
        printf("driver_deactivate: Geçersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_deactivate: Sürücü bulunamadı: %s\n", name);
        return -1;
    }
    
    // Aktif listede mi kontrol et
    if (!find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_deactivate: Sürücü aktif değildi: %s\n", name);
        return -1;
    }
    
    // Sürücüyü kapat
    if (driver->shutdown) {
        driver->shutdown();
    }
    
    // Aktif listeden çıkar
    if (remove_driver_from_list(&driver_mgr.active_list, driver) == 0) {
        driver_mgr.active_count--;
        printf("Sürücü devre dışı bırakıldı: %s\n", name);
        return 0;
    }
    
    printf("driver_deactivate: Sürücü aktif listeden kaldırılamadı: %s\n", name);
    return -1;
}

int driver_deactivate_by_id(uint32_t unique_id) {
    driver_t* driver = driver_get_by_unique_id(unique_id);
    if (!driver) {
        printf("driver_deactivate_by_id: Sürücü bulunamadı: ID %d\n", unique_id);
        return -1;
    }
    
    // Aktif listede mi kontrol et
    if (!find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_deactivate_by_id: Sürücü aktif değildi: ID %d\n", unique_id);
        return -1;
    }
    
    // Sürücüyü kapat
    if (driver->shutdown) {
        driver->shutdown();
    }
    
    // Aktif listeden çıkar
    if (remove_driver_from_list(&driver_mgr.active_list, driver) == 0) {
        driver_mgr.active_count--;
        printf("Sürücü devre dışı bırakıldı: ID %d\n", unique_id);
        return 0;
    }
    
    printf("driver_deactivate_by_id: Sürücü aktif listeden kaldırılamadı: ID %d\n", unique_id);
    return -1;
}

driver_t* driver_get(const char* name) {
    if (!name || !*name) {
        printf("driver_get: Geçersiz isim\n");
        return 0;
    }
    
    driver_node_t* current = driver_mgr.driver_list;
    while (current) {
        if (current->driver && current->driver->name && 
            strcmp(current->driver->name, name) == 0) {
            return current->driver;
        }
        current = current->next;
    }
    return 0;
}

driver_t* driver_find_by_id(uint16_t vendor_id, uint16_t device_id) {
    driver_node_t* current = driver_mgr.driver_list;
    while (current) {
        if (current->driver && 
            current->driver->vendor_id == vendor_id && 
            current->driver->device_id == device_id) {
            return current->driver;
        }
        current = current->next;
    }
    return 0;
}

driver_t* driver_get_by_class(driver_class_t class) {
    driver_node_t* current = driver_mgr.driver_list;
    while (current) {
        if (current->driver && current->driver->class == class) {
            return current->driver;
        }
        current = current->next;
    }
    return 0;
}

driver_t* driver_get_by_unique_id(uint32_t unique_id) {
    driver_node_t* current = driver_mgr.driver_list;
    while (current) {
        if (current->driver && current->driver->unique_id == unique_id) {
            return current->driver;
        }
        current = current->next;
    }
    return 0;
}

driver_t* driver_get_by_type(driver_type_t type) {
    driver_node_t* current = driver_mgr.driver_list;
    while (current) {
        if (current->driver && current->driver->type == type) {
            return current->driver;
        }
        current = current->next;
    }
    return 0;
}

void driver_list_all() {
    printf("\n=== Kayıtlı Sürüciler ===\n");
    driver_node_t* current = driver_mgr.driver_list;
    int count = 1;
    while (current) {
        if (current->driver) {
            printf("%d. %s (ID: %d, Tip: %d, %04X:%04X)\n", 
                   count++, current->driver->name, current->driver->unique_id, 
                   current->driver->type, current->driver->vendor_id, current->driver->device_id);
        }
        current = current->next;
    }
    printf("========================\n");
}

void driver_list_active() {
    printf("\n=== Aktif Sürüciler ===\n");
    driver_node_t* current = driver_mgr.active_list;
    int count = 1;
    while (current) {
        if (current->driver) {
            printf("%d. %s (ID: %d, Tip: %d)\n", 
                   count++, current->driver->name, current->driver->unique_id, 
                   current->driver->type);
        }
        current = current->next;
    }
    printf("=======================\n");
}

int driver_unregister(const char* name) {
    if (!name || !*name) {
        printf("driver_unregister: Geçersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_unregister: Sürücü bulunamadı: %s\n", name);
        return -1;
    }
    
    // Önce devre dışı bırak (aktif listeden çıkar)
    if (find_in_list(driver_mgr.active_list, driver)) {
        driver_deactivate(name);
    }
    
    // Kayıttan sil
    if (remove_driver_from_list(&driver_mgr.driver_list, driver) == 0) {
        driver_mgr.driver_count--;
        printf("Sürücü kayıttan silindi: %s\n", name);
        return 0;
    }
    
    printf("driver_unregister: Sürücü kayıttan kaldırılamadı: %s\n", name);
    return -1;
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
