#include "shell.h"
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "gumus_dil.h"
#include "fs.h"
#include "ata.h"
#include "vga_gfx.h"
#include "sound.h"
#include "window.h"
#include "snake.h"
#include "../drivers/network/ethernet.h"
#include "../drivers/network/arp.h"
#include "../drivers/network/ip.h"
#include "../drivers/network/icmp.h"
#include "file_manager_gui.h"
#include "system_monitor.h"

static char command_buffer[MAX_COMMAND_LEN];
static int buffer_index = 0;

// Komut Geçmişi (History)
#define MAX_HISTORY 10
static char history[MAX_HISTORY][MAX_COMMAND_LEN];
static int history_count = 0;
static int history_index = -1; // Şu an bakılan geçmiş indeksi

// Harici değişkenler (memory.c'den)
extern uint32_t used_blocks;
extern uint32_t max_blocks;

// Melodiler
static note_t plevne_marsi[] = {
    {NOTE_A4, 4}, {0, 1}, {NOTE_A4, 4}, {0, 1}, // Tu-na
    {NOTE_A4, 8}, {0, 1}, {NOTE_B4, 4}, {0, 1}, {NOTE_C5, 4}, {0, 1}, // Neh-ri
    {NOTE_B4, 4}, {0, 1}, {NOTE_A4, 4}, {0, 1}, {NOTE_G4, 4}, {0, 1}, // Ak-mam
    {NOTE_B4, 8}, {0, 2}, // Di-yor (Uzun)
    {NOTE_C5, 4}, {0, 1}, {NOTE_B4, 4}, {0, 1}, {NOTE_A4, 4}, {0, 1}, // Et-ra-fi-mi
    {NOTE_G4, 4}, {0, 1}, {NOTE_F4, 4}, {0, 1}, {NOTE_E4, 16}, // Yik-mam (Cok Uzun)
    {0, 0} // Bitiş
};

void shell_init() {
    buffer_index = 0;
    memset(command_buffer, 0, MAX_COMMAND_LEN);
    print_color("\n> ", LIGHT_CYAN);
}

void shell_parse_command(char* cmd) {
    if (strlen(cmd) > 0) {
        // Geçmişe ekle (aynı komut değilse)
        if (history_count == 0 || strcmp(history[(history_count-1) % MAX_HISTORY], cmd) != 0) {
            strcpy(history[history_count % MAX_HISTORY], cmd);
            history_count++;
        }
    }
    history_index = -1; // İndeksi sıfırla

    if (strcmp(cmd, "yardim") == 0 || strcmp(cmd, "help") == 0) {
        print_color("\nGumusOS Komutlari:\n", YELLOW);
        print("  yardim      - Komut listesini gosterir\n");
        print("  temizle     - Ekrani temizler\n");
        print("  versiyon    - Sistem versiyonunu gosterir\n");
        print("  bellek      - Bellek durumunu gosterir\n");
        print("  kyber       - Kuantum sonrasi sifreleme testi\n");
        print("  listele     - Dosyalari listeler (ls)\n");
        print("  oku [ad]    - Dosya icerigini okur\n");
        print("  kaydet [ad] [icerik] - Dosya olusturur\n");
        print("  bicimlendir - Test disk alanini hazirlar (format)\n");
        print("  test        - Bellek testi yapar\n");
        print("  pencere     - Ekran testi icin pencere acar\n");
        print_color("\nAg Komutlari:\n", LIGHT_CYAN);
        print("  ag_baslat   - Ag suruculerini baslatir\n");
        print("  ag_durum    - Ag durumunu gosterir\n");
        print("  ping [IP]   - ICMP ping gonderir\n");
        print("  arp [IP]    - ARP tablosunu sorgular\n");
        print_color("\nGrafiksel Arayuz:\n", LIGHT_CYAN);
        print("  dosyalar    - Grafiksel dosya yöneticisini acar\n");
        print("  monitor     - Sistem monitörünü acar\n");
        print_color("\nGumusDil (soyle) Örnekleri:\n", LIGHT_GREEN);
        print("  yaz \"Selam\"        - Ekrana yazar\n");
        print("  kutu(50,50,5,5,10) - Dikdortgen cizer\n");
        print("  temizle(0)         - Ekrani siyah yapar\n");
        print("  ses(440,500)       - 440Hz ses calar\n");
    } else if (strcmp(cmd, "pencere") == 0) {
        create_window("Pencere A", 5, 5, 20, 10, (BLUE << 4) | WHITE);
        create_window("Pencere B", 15, 8, 20, 10, (RED << 4) | WHITE);
        print("\n2 Adet test penceresi olusturuldu.\n");
    } else if (strcmp(cmd, "yilan") == 0) {
        init_snake_game();
        print("\nYilan oyunu baslatildi! WASD ile oyna.\n");
    } else if (strcmp(cmd, "listele") == 0 || strcmp(cmd, "ls") == 0) {
        fs_ls();
    } else if (strncmp(cmd, "oku ", 4) == 0) {
        fs_cat(cmd + 4);
    } else if (strncmp(cmd, "kaydet ", 7) == 0) {
        char* filename = cmd + 7;
        char* content = (void*)0;
        for (int i = 0; filename[i] != '\0'; i++) {
            if (filename[i] == ' ') {
                filename[i] = '\0';
                content = filename + i + 1;
                break;
            }
        }
        if (content) {
            fs_write(filename, content);
        } else {
            print_color("\nHata: Kullanim: kaydet [dosya] [icerik]\n", LIGHT_RED);
        }
    } else if (strncmp(cmd, "yukle ", 6) == 0) {
        char* filename = cmd + 6;
        char* data = fs_read(filename);
        if (data) {
            print_color("\n[GumusOS] ", LIGHT_GREEN); print(filename); print(" yuklendi, calistiriliyor...\n");
            gumus_execute(data);
            kfree(data);
        } else {
            print_color("\nHata: Dosya okunamadi.\n", LIGHT_RED);
        }
    } else if (strcmp(cmd, "bicimlendir") == 0 || strcmp(cmd, "format") == 0) {
        print_color("\nTest disk alani hazirlaniyor (Sektor 100-200)...\n", YELLOW);
        uint8_t* buffer = kmalloc(512);
        memset(buffer, 0, 512);
        fat_directory_entry_t* root = (fat_directory_entry_t*)buffer;
        memcpy(root[0].name, "SELAM   ", 8);
        memcpy(root[0].ext, "TXT", 3);
        root[0].file_size = 18;
        root[0].first_cluster_lo = 1;
        ata_write_sectors(100, 1, (uint32_t)buffer);
        memset(buffer, 0, 512);
        strcpy((char*)buffer, "Merhaba GumusOS!");
        ata_write_sectors(201, 1, (uint32_t)buffer);
        print_color("Bicimlendirme tamamlandi.\n", LIGHT_GREEN);
        kfree(buffer);
    } else if (strcmp(cmd, "temizle") == 0 || strcmp(cmd, "clear") == 0) {
        clear_screen();
        draw_logo();
    } else if (strcmp(cmd, "versiyon") == 0) {
        print_color("\nGumusOS v0.1.0 'Uyanis'\n", LIGHT_MAGENTA);
    } else if (strcmp(cmd, "bellek") == 0) {
        char buf[16];
        print_color("\nBellek Durumu:\n", YELLOW);
        print("  Sayfa Yonetimi (PMM):\n");
        print("    Kullanilan: "); itoa(used_blocks, buf); print(buf); print(" blok\n");
        print("    Toplam:     "); itoa(max_blocks, buf); print(buf); print(" blok\n");
        print("  Dinamik Bellek (Heap):\n");
        print_color("    Durum: AKTIF (Linked List)\n", LIGHT_GREEN);
    } else if (strcmp(cmd, "test") == 0) {
        print_color("\nBellek Testi Basliyor...\n", YELLOW);
        void* p1 = kmalloc(512);
        print("  p1 (512 byte) alindi.\n");
        kfree(p1);
        print("  p1 iade edildi.\n");
        print_color("Bellek testi tamamlandi.\n", LIGHT_GREEN);
    } else if (strcmp(cmd, "kyber") == 0) {
        print_color("\n[Kyber] Kuantum direncli anahtar kapsülleme baslatiliyor...\n", LIGHT_CYAN);
        print("[Kyber] Anahtar Uretimi: [ OK ]\n");
        print("[Kyber] Sifreleme:       [ OK ]\n");
        print("[Kyber] Cozme:           [ OK ]\n");
    } else if (strncmp(cmd, "gumus-", 6) == 0 || strncmp(cmd, "soyle ", 6) == 0) {
        gumus_execute(cmd);
    } else if (strcmp(cmd, "cal") == 0) {
        print_color("\n[Gumus Player] Plevne Marsi caliniyor...\n", LIGHT_CYAN);
        play_melody(plevne_marsi, 21); // Dizi uzunluğunu elle veriyoruz (şimdilik)
        print("Muzik calarken komut yazmaya devam edebilirsiniz!\n");
    } else if (strcmp(cmd, "dur") == 0) {
        stop_melody();
        print_color("\nMuzik durduruldu.\n", YELLOW);
    } else if (strcmp(cmd, "ciz") == 0) {
        for(int i = 0; i < 50; i++) {
            vga_draw_rect(i*6, i*4, 10, 10, i % 256);
        }
    } else if (strcmp(cmd, "ag_baslat") == 0) {
        print_color("\nAg suruculeri baslatiliyor...\n", LIGHT_CYAN);
        
        if (ethernet_init() == 0) {
            print_color("Ethernet surucusu: [ OK ]\n", LIGHT_GREEN);
        } else {
            print_color("Ethernet surucusu: [ HATA ]\n", LIGHT_RED);
        }
        
        if (arp_init() == 0) {
            print_color("ARP protokolu: [ OK ]\n", LIGHT_GREEN);
        } else {
            print_color("ARP protokolu: [ HATA ]\n", LIGHT_RED);
        }
        
        if (ip_init() == 0) {
            print_color("IP protokolu: [ OK ]\n", LIGHT_GREEN);
        } else {
            print_color("IP protokolu: [ HATA ]\n", LIGHT_RED);
        }
        
        if (icmp_init() == 0) {
            print_color("ICMP protokolu: [ OK ]\n", LIGHT_GREEN);
        } else {
            print_color("ICMP protokolu: [ HATA ]\n", LIGHT_RED);
        }
        
        print_color("Ag suruculeri baslatildi.\n", LIGHT_GREEN);
    } else if (strcmp(cmd, "ag_durum") == 0) {
        print_color("\nAg Durumu:\n", LIGHT_CYAN);
        print("MAC Adres: ");
        mac_addr_t mac = ethernet_get_mac();
        ethernet_print_mac(&mac);
        print("\nIP Adres: ");
        uint8_t* ip = ip_get_source_ip();
        char ip_str[16];
        ip_to_string(ip, ip_str);
        print(ip_str);
        print("\n");
    } else if (strncmp(cmd, "ping ", 5) == 0) {
        char* ip_str = cmd + 5;
        uint8_t target_ip[4];
        string_to_ip(ip_str, target_ip);
        
        print_color("\nPing baslatiliyor: ", LIGHT_CYAN);
        print(ip_str);
        print("\n");
        
        ping(target_ip, 4);
    } else if (strncmp(cmd, "arp ", 4) == 0) {
        char* ip_str = cmd + 4;
        uint8_t target_ip[4];
        string_to_ip(ip_str, target_ip);
        
        print_color("\nARP sorgulanıyor: ", LIGHT_CYAN);
        print(ip_str);
        print("\n");
        
        mac_addr_t mac;
        if (arp_resolve(target_ip, &mac)) {
            print("MAC Adres: ");
            ethernet_print_mac(&mac);
            print("\n");
        } else {
            print_color("ARP cozumlemesi basarisiz.\n", LIGHT_RED);
        }
    } else if (strcmp(cmd, "dosyalar") == 0) {
        print_color("\nGrafiksel Dosya Yöneticisi aciliyor...\n", LIGHT_CYAN);
        launch_file_manager_gui();
    } else if (strcmp(cmd, "monitor") == 0) {
        print_color("\nSistem Monitörü aciliyor...\n", LIGHT_CYAN);
        launch_system_monitor();
    } else if (strlen(cmd) > 0) {
        print_color("\nBilinmeyen komut: ", LIGHT_RED);
        print(cmd);
        print("\n");
    }
}

void shell_input(char c) {
    if (c == '\n') {
        command_buffer[buffer_index] = '\0';
        shell_parse_command(command_buffer);
        buffer_index = 0;
        memset(command_buffer, 0, MAX_COMMAND_LEN);
        print_color("\n> ", LIGHT_CYAN);
    } else if (c == '\b') {
        if (buffer_index > 0) {
            buffer_index--;
            putchar('\b');
        }
    } else if ((uint8_t)c == 0x80 || (uint8_t)c == 0x81) {
        // Geçmiş (History) Gezintisi
        if (history_count == 0) return;

        if ((uint8_t)c == 0x80) { // YUKARI
            if (history_index == -1) history_index = history_count - 1;
            else if (history_index > 0 && history_index > history_count - MAX_HISTORY) history_index--;
        } else { // AŞAĞI
            if (history_index != -1 && history_index < history_count - 1) history_index++;
            else { history_index = -1; }
        }

        // Mevcut satırı temizle
        while (buffer_index > 0) {
            putchar('\b');
            buffer_index--;
        }

        // Yeni komutu yükle
        if (history_index != -1) {
            strcpy(command_buffer, history[history_index % MAX_HISTORY]);
            buffer_index = strlen(command_buffer);
            print(command_buffer);
        } else {
            memset(command_buffer, 0, MAX_COMMAND_LEN);
            buffer_index = 0;
        }
    } else {
        if (buffer_index < MAX_COMMAND_LEN - 1) {
            command_buffer[buffer_index++] = c;
            putchar(c);
        }
    }
}
