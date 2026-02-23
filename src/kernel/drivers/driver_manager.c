#include "driver.h"
#include "string.h"
#include "hardware_detect.h"
#include "memory.h"
#include "printf.h"

// Global sÃ¼rÃ¼cÃ¼ yÃ¶neticisi
static driver_manager_t driver_mgr;

// YardÄ±mcÄ± fonksiyonlar
static driver_node_t* create_driver_node(driver_t* driver) {
    driver_node_t* node = malloc(sizeof(driver_node_t));
    if (!node) {
        printf("KRÄ°TÄ°K HATA: Bellek yetersiz - driver_node oluÅŸturulamadÄ±!\n");
        printf("Sistem panik moduna geÃ§iyor...\n");
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
    printf("SÃ¼rÃ¼cÃ¼ yÃ¶neticisi temizleniyor...\n");
    
    // Ã–nce tÃ¼m aktif sÃ¼rÃ¼cÃ¼leri kapat
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
    
    // KayÄ±tlÄ± listeyi temizle
    free_driver_list(&driver_mgr.driver_list);
    driver_mgr.driver_count = 0;
    
    printf("SÃ¼rÃ¼cÃ¼ yÃ¶neticisi temizlendi\n");
}

void driver_manager_init() {
    driver_mgr.driver_list = NULL;
    driver_mgr.active_list = NULL;
    driver_mgr.driver_count = 0;
    driver_mgr.active_count = 0;
    driver_mgr.next_unique_id = 1;
    
    printf("SÃ¼rÃ¼cÃ¼ yÃ¶neticisi baÅŸlatÄ±ldÄ±\n");
}

void driver_register(driver_t* driver) {
    if (!driver) {
        printf("HATA: BoÅŸ sÃ¼rÃ¼cÃ¼ kaydedilemez\n");
        return;
    }
    
    // Unique ID ata
    driver->unique_id = driver_mgr.next_unique_id++;
    
    // Listeye ekle
    add_driver_to_list(&driver_mgr.driver_list, driver);
    driver_mgr.driver_count++;
    
    printf("SÃ¼rÃ¼cÃ¼ kaydedildi: %s (ID: %d, %04X:%04X)\n", 
           driver->name, driver->unique_id, driver->vendor_id, driver->device_id);
}

int driver_activate(const char* name) {
    if (!name || !*name) {
        printf("driver_activate: GeÃ§ersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_activate: SÃ¼rÃ¼cÃ¼ bulunamadÄ±: %s\n", name);
        return -1;
    }
    
    // Zaten aktif mi kontrol et
    if (find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_activate: SÃ¼rÃ¼cÃ¼ zaten aktif: %s\n", name);
        return 0;
    }
    
    // SÃ¼rÃ¼cÃ¼yÃ¼ baÅŸlat
    if (driver->init && driver->init() == 0) {
        // Aktif listeye ekle
        add_driver_to_list(&driver_mgr.active_list, driver);
        driver_mgr.active_count++;
        printf("SÃ¼rÃ¼cÃ¼ aktifleÅŸtirildi: %s\n", name);
        return 0;
    } else {
        printf("driver_activate: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±lamadÄ±: %s\n", name);
        return -1;
    }
}

int driver_activate_by_id(uint32_t unique_id) {
    driver_t* driver = driver_get_by_unique_id(unique_id);
    if (!driver) {
        printf("driver_activate_by_id: SÃ¼rÃ¼cÃ¼ bulunamadÄ±: ID %d\n", unique_id);
        return -1;
    }
    
    // Zaten aktif mi kontrol et
    if (find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_activate_by_id: SÃ¼rÃ¼cÃ¼ zaten aktif: ID %d\n", unique_id);
        return 0;
    }
    
    // SÃ¼rÃ¼cÃ¼yÃ¼ baÅŸlat
    if (driver->init && driver->init() == 0) {
        // Aktif listeye ekle
        add_driver_to_list(&driver_mgr.active_list, driver);
        driver_mgr.active_count++;
        printf("SÃ¼rÃ¼cÃ¼ aktifleÅŸtirildi: %s (ID: %d)\n", driver->name, unique_id);
        return 0;
    } else {
        printf("driver_activate_by_id: SÃ¼rÃ¼cÃ¼ baÅŸlatÄ±lamadÄ±: ID %d\n", unique_id);
        return -1;
    }
}

int driver_deactivate(const char* name) {
    if (!name || !*name) {
        printf("driver_deactivate: GeÃ§ersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_deactivate: SÃ¼rÃ¼cÃ¼ bulunamadÄ±: %s\n", name);
        return -1;
    }
    
    // Aktif listede mi kontrol et
    if (!find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_deactivate: SÃ¼rÃ¼cÃ¼ aktif deÄŸildi: %s\n", name);
        return -1;
    }
    
    // SÃ¼rÃ¼cÃ¼yÃ¼ kapat
    if (driver->shutdown) {
        driver->shutdown();
    }
    
    // Aktif listeden Ã§Ä±kar
    if (remove_driver_from_list(&driver_mgr.active_list, driver) == 0) {
        driver_mgr.active_count--;
        printf("SÃ¼rÃ¼cÃ¼ devre dÄ±ÅŸÄ± bÄ±rakÄ±ldÄ±: %s\n", name);
        return 0;
    }
    
    printf("driver_deactivate: SÃ¼rÃ¼cÃ¼ aktif listeden kaldÄ±rÄ±lamadÄ±: %s\n", name);
    return -1;
}

int driver_deactivate_by_id(uint32_t unique_id) {
    driver_t* driver = driver_get_by_unique_id(unique_id);
    if (!driver) {
        printf("driver_deactivate_by_id: SÃ¼rÃ¼cÃ¼ bulunamadÄ±: ID %d\n", unique_id);
        return -1;
    }
    
    // Aktif listede mi kontrol et
    if (!find_in_list(driver_mgr.active_list, driver)) {
        printf("driver_deactivate_by_id: SÃ¼rÃ¼cÃ¼ aktif deÄŸildi: ID %d\n", unique_id);
        return -1;
    }
    
    // SÃ¼rÃ¼cÃ¼yÃ¼ kapat
    if (driver->shutdown) {
        driver->shutdown();
    }
    
    // Aktif listeden Ã§Ä±kar
    if (remove_driver_from_list(&driver_mgr.active_list, driver) == 0) {
        driver_mgr.active_count--;
        printf("SÃ¼rÃ¼cÃ¼ devre dÄ±ÅŸÄ± bÄ±rakÄ±ldÄ±: ID %d\n", unique_id);
        return 0;
    }
    
    printf("driver_deactivate_by_id: SÃ¼rÃ¼cÃ¼ aktif listeden kaldÄ±rÄ±lamadÄ±: ID %d\n", unique_id);
    return -1;
}

driver_t* driver_get(const char* name) {
    if (!name || !*name) {
        printf("driver_get: GeÃ§ersiz isim\n");
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
    printf("\n=== KayÄ±tlÄ± SÃ¼rÃ¼ciler ===\n");
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
    printf("\n=== Aktif SÃ¼rÃ¼ciler ===\n");
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
        printf("driver_unregister: GeÃ§ersiz isim\n");
        return -1;
    }
    
    driver_t* driver = driver_get(name);
    if (!driver) {
        printf("driver_unregister: SÃ¼rÃ¼cÃ¼ bulunamadÄ±: %s\n", name);
        return -1;
    }
    
    // Ã–nce devre dÄ±ÅŸÄ± bÄ±rak (aktif listeden Ã§Ä±kar)
    if (find_in_list(driver_mgr.active_list, driver)) {
        driver_deactivate(name);
    }
    
    // KayÄ±ttan sil
    if (remove_driver_from_list(&driver_mgr.driver_list, driver) == 0) {
        driver_mgr.driver_count--;
        printf("SÃ¼rÃ¼cÃ¼ kayÄ±ttan silindi: %s\n", name);
        return 0;
    }
    
    printf("driver_unregister: SÃ¼rÃ¼cÃ¼ kayÄ±ttan kaldÄ±rÄ±lamadÄ±: %s\n", name);
    return -1;
}

int driver_auto_load_all() {
    hardware_info_t* hw_info = hardware_get_info();
    int loaded_count = 0;
    
    printf("Otomatik sÃ¼rÃ¼cÃ¼ yÃ¼klemesi baÅŸlatÄ±lÄ±yor...\n");
    
    for(int i=0; i<hw_info->pci_device_count; i++) {
        if (hardware_load_driver_for_device(&hw_info->pci_devices[i]) == 0) {
            loaded_count++;
        }
    }
    
    printf("Toplam %d sÃ¼rÃ¼cÃ¼ yÃ¼klendi.\n", loaded_count);
    return loaded_count;
}
