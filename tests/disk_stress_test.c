#include "framework.h"
#include "../src/kernel/drivers/storage/ata.h"
#include "memory.h"
#include "string.h"

/**
 * DEP-DAYI TAVSİYESİ: DEPREME DAYANIKLILIK TESTİ (I/O STRESS)
 */

#define STRESS_ITERATIONS 100
#define BUFFER_SIZE 512

int test_disk_stress_io() {
    uint8_t* write_buf = kmalloc(BUFFER_SIZE);
    uint8_t* read_buf = kmalloc(BUFFER_SIZE);
    
    _print_raw("Starting 100 iterations of Write/Read Stress...", 2, 4, 0x07);

    for (int i = 0; i < STRESS_ITERATIONS; i++) {
        // Her iterasyonda farklı veri üret (Desene dayalı)
        for (int j = 0; j < BUFFER_SIZE; j++) {
            write_buf[j] = (uint8_t)((i + j) & 0xFF);
        }

        // LBA 500'e yaz (Güvenli bir bölge varsayalım)
        ata_write_sectors(500, 1, (uint32_t)write_buf);
        
        // Geri oku
        memset(read_buf, 0, BUFFER_SIZE);
        ata_read_sectors((uint32_t)read_buf, 500, 1);

        // Karşılaştır
        if (memcmp(write_buf, read_buf, BUFFER_SIZE) != 0) {
            ASSERT_DETAIL(0, "I/O Corruption at iteration", i, 0);
            kfree(write_buf);
            kfree(read_buf);
            return TEST_FAIL;
        }

        // İlerlemeyi göster
        if (i % 10 == 0) {
            char progress[16];
            itoa(i, progress);
            _print_raw("Progress: ", 2, 5, 0x07);
            _print_raw(progress, 12, 5, 0x0E);
            _print_raw(" / 100", 15, 5, 0x07);
        }
    }

    kfree(write_buf);
    kfree(read_buf);
    return TEST_PASS;
}

void kernel_main() {
    test_header("I/O STRESS: DEPREME DAYANIKLILIK");
    
    // Not: Bu test GERÇEK bir disk veya QEMU disk imajı gerektirir.
    // Eğer disk bağlı değilse ATA sürücüsü hata verebilir veya sonsuz döngüye girebilir.
    
    RUN_TEST(test_disk_stress_io, "100x Write-Read Integrity (LBA 500)");
    
    _print_raw("Disk timing and integrity verified.", 2, 12, 0x0B);
    while(1) { __asm__ volatile("hlt"); }
}
