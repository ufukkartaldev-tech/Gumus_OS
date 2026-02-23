#include "driver.h"
#include "string.h"
#include "memory.h"

// NULL SÃ¼rÃ¼cÃ¼sÃ¼ (Hepsini yer, hiÃ§bir ÅŸey vermez)
static int null_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    return size;
}

static int null_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    return 0;
}

static driver_t null_driver = {
    .name = "null",
    .type = DRIVER_TYPE_CHAR,
    .class = PCI_CLASS_UNCLASSIFIED,
    .init = 0,
    .read = null_driver_read,
    .write = null_driver_write,
    .ioctl = 0,
    .shutdown = 0
};

// ZERO SÃ¼rÃ¼cÃ¼sÃ¼ (Her ÅŸeyi sÄ±fÄ±r dÃ¶ndÃ¼rÃ¼r)
static int zero_driver_write(void* buffer, uint32_t size, uint32_t offset) {
    return size;
}

static int zero_driver_read(void* buffer, uint32_t size, uint32_t offset) {
    memset(buffer, 0, size);
    return size;
}

static driver_t zero_driver = {
    .name = "zero",
    .type = DRIVER_TYPE_CHAR,
    .class = PCI_CLASS_UNCLASSIFIED,
    .init = 0,
    .read = zero_driver_read,
    .write = zero_driver_write,
    .ioctl = 0,
    .shutdown = 0
};

void pseudo_drivers_init() {
    driver_register(&null_driver);
    driver_register(&zero_driver);
}
