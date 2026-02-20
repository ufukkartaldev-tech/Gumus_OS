#include "driver.h"
#include "string.h"

#define MAX_DRIVERS 32

static driver_t* drivers[MAX_DRIVERS];
static int driver_count = 0;

void driver_manager_init() {
    for(int i=0; i<MAX_DRIVERS; i++) drivers[i] = 0;
    driver_count = 0;
}

void driver_register(driver_t* driver) {
    if (driver_count < MAX_DRIVERS) {
        drivers[driver_count++] = driver;
        // Sürücüyü başlat
        if (driver->init) {
            driver->init();
        }
    }
}

driver_t* driver_get(const char* name) {
    for(int i=0; i<driver_count; i++) {
        if (strcmp(drivers[i]->name, name) == 0) {
            return drivers[i];
        }
    }
    return 0;
}
