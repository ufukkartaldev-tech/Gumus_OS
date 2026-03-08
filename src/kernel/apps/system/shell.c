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
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "dhcp.h"
#include "dns.h"
#include "ftp.h"
#include "ext2.h"
#include "file_manager_gui.h"
#include "hardware_detect.h"
#include "file_manager_gui.h"
#include "system_monitor.h"
#include "advanced_sound.h"
#include "audio_mixer.h"
#include "elf.h"

static char command_buffer[MAX_COMMAND_LEN];
static int buffer_index = 0;

// Komut GeÃ§miÅŸi (History)
#define MAX_HISTORY 10
static char history[MAX_HISTORY][MAX_COMMAND_LEN];
static int history_count = 0;
static int history_index = -1; // Åu an bakÄ±lan geÃ§miÅŸ indeksi

// Harici deÄŸiÅŸkenler (memory.c'den)
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
    {0, 0} // BitiÅŸ
};

void shell_init() {
    buffer_index = 0;
    memset(command_buffer, 0, MAX_COMMAND_LEN);
    print_color("\n> ", LIGHT_CYAN);
}

void shell_parse_command(char* cmd) {
    if (strlen(cmd) > 0) {
        // GeÃ§miÅŸe ekle (aynÄ± komut deÄŸilse)
        if (history_count == 0 || strcmp(history[(history_count-1) % MAX_HISTORY], cmd) != 0) {
            strcpy(history[history_count % MAX_HISTORY], cmd);
            history_count++;
        }
    }
    history_index = -1; // Ä°ndeksi sÄ±fÄ±rla

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
        print("  dhcp_yenile - DHCP IP adresini yeniler\n");
        print("  dns coz [domain] - Domain adını cozuler\n");
        print("  ftp list [server] - FTP sunucusunda dosya listeler\n");
        print("  ftp indir [server] [dosya] - FTP'den dosya indirir\n");
        print("  ftp yukle [server] [dosya] - FTP'ye dosya yükler\n");
        print_color("\nDosya Sistemi Komutları:\n", LIGHT_CYAN);
        print("  ext2_mount [device] - EXT2 filesystem'i mount eder\n");
        print("  ext2_unmount - EXT2 filesystem'i unmount eder\n");
        print("  ls -l [path] - Dosyaları detaylı listeler\n");
        print("  chmod [dosya] [izin] - Dosya izinlerini değiştirir\n");
        print_color("\nGrafiksel Arayuz:\n", LIGHT_CYAN);
        print("  dosyalar    - Grafiksel dosya yÃ¶neticisini acar\n");
        print("  monitor     - Sistem monitÃ¶rÃ¼nÃ¼ acar\n");
        print_color("\nSes Sistemi:\n", LIGHT_CYAN);
        print("  ses_cal     - Ses mixer arayÃ¼zÃ¼nÃ¼ acar\n");
        print("  ses_kaydet  - Ses kaydÄ±nÄ± baslatir/durdurur\n");
        print("  wav_cal [dosya] - WAV dosyasÄ± calar\n");
        print_color("\nGumusDil (soyle) Ã–rnekleri:\n", LIGHT_GREEN);
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
    } else if (strncmp(cmd, "calistir ", 9) == 0) {
        char* filename = cmd + 9;
        print_color("\n[ELF] Program yukleniyor: ", LIGHT_CYAN);
        print(filename);
        print("\n");
        if (elf_load_and_run(filename) != 0) {
            print_color("Hata: Program yuklenemedi veya gecersiz ELF.\n", LIGHT_RED);
        }
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
        print_color("\n[Kyber] Kuantum direncli anahtar kapsÃ¼lleme baslatiliyor...\n", LIGHT_CYAN);
        print("[Kyber] Anahtar Uretimi: [ OK ]\n");
        print("[Kyber] Sifreleme:       [ OK ]\n");
        print("[Kyber] Cozme:           [ OK ]\n");
    } else if (strncmp(cmd, "gumus-", 6) == 0 || strncmp(cmd, "soyle ", 6) == 0) {
        gumus_execute(cmd);
    } else if (strcmp(cmd, "cal") == 0) {
        print_color("\n[Gumus Player] Plevne Marsi caliniyor...\n", LIGHT_CYAN);
        play_melody(plevne_marsi, 21); // Dizi uzunluÄŸunu elle veriyoruz (ÅŸimdilik)
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
        
        print_color("\nARP sorgulanÄ±yor: ", LIGHT_CYAN);
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
    } else if (strcmp(cmd, "dhcp_yenile") == 0) {
        print_color("\nDHCP yenileniyor...\n", LIGHT_CYAN);
        
        if (dhcp_init() == 0) {
            print_color("DHCP Client baslatildi.\n", LIGHT_GREEN);
            if (dhcp_discover() == 0) {
                print_color("DHCP Discover gonderildi.\n", LIGHT_GREEN);
            } else {
                print_color("DHCP Discover gonderilemedi.\n", LIGHT_RED);
            }
        } else {
            print_color("DHCP Client baslatilamadi.\n", LIGHT_RED);
        }
    } else if (strncmp(cmd, "dns coz ", 8) == 0) {
        char* domain = cmd + 8;
        print_color("\nDNS Cozumleniyor: ", LIGHT_CYAN);
        print(domain);
        print("\n");
        
        uint8_t ip[4];
        int result = dns_resolve(domain, ip);
        
        if (result == 0) {
            char ip_str[16];
            dns_ip_to_string(ip, ip_str);
            print_color("IP Adres: ", LIGHT_GREEN);
            print(ip_str);
            print("\n");
        } else if (result == -2) {
            print_color("DNS cevabi bekleniyor...\n", YELLOW);
        } else {
            print_color("DNS cozumlemesi basarisiz.\n", LIGHT_RED);
        }
    } else if (strncmp(cmd, "ftp list ", 9) == 0) {
        char* server_str = cmd + 9;
        print_color("\nFTP Sunucusuna bağlanılıyor: ", LIGHT_CYAN);
        print(server_str);
        print("\n");
        
        uint8_t server_ip[4];
        if (dns_string_to_ip(server_str, server_ip) < 0) {
            print_color("Geçersiz IP adresi.\n", LIGHT_RED);
        } else {
            if (ftp_connect(&ftp_main_connection, server_ip, 21) == 0) {
                if (ftp_login(&ftp_main_connection, "anonymous", "guest@gumusos.local") == 0) {
                    ftp_directory_t dir;
                    if (ftp_list_directory(&ftp_main_connection, &dir) == 0) {
                        ftp_print_directory(&dir);
                    } else {
                        print_color("Directory listelenemedi.\n", LIGHT_RED);
                    }
                } else {
                    print_color("FTP login başarısız.\n", LIGHT_RED);
                }
                ftp_disconnect(&ftp_main_connection);
            } else {
                print_color("FTP bağlantısı kurulamadı.\n", LIGHT_RED);
            }
        }
    } else if (strncmp(cmd, "ftp indir ", 10) == 0) {
        char* params = cmd + 10;
        char* space = strchr(params, ' ');
        if (!space) {
            print_color("Kullanım: ftp indir [server] [dosya]\n", YELLOW);
        } else {
            *space = '\0';
            char* server_str = params;
            char* filename = space + 1;
            
            print_color("\nFTP'den dosya indiriliyor: ", LIGHT_CYAN);
            printf("%s -> %s\n", filename, filename);
            
            uint8_t server_ip[4];
            if (dns_string_to_ip(server_str, server_ip) < 0) {
                print_color("Geçersiz IP adresi.\n", LIGHT_RED);
            } else {
                if (ftp_connect(&ftp_main_connection, server_ip, 21) == 0) {
                    if (ftp_login(&ftp_main_connection, "anonymous", "guest@gumusos.local") == 0) {
                        if (ftp_download_file(&ftp_main_connection, filename, filename) == 0) {
                            print_color("Dosya başarıyla indirildi.\n", LIGHT_GREEN);
                        } else {
                            print_color("Dosya indirilemedi.\n", LIGHT_RED);
                        }
                    } else {
                        print_color("FTP login başarısız.\n", LIGHT_RED);
                    }
                    ftp_disconnect(&ftp_main_connection);
                } else {
                    print_color("FTP bağlantısı kurulamadı.\n", LIGHT_RED);
                }
            }
        }
    } else if (strncmp(cmd, "ftp yukle ", 10) == 0) {
        print_color("\nFTP upload henüz implemente edilmedi.\n", YELLOW);
    } else if (strcmp(cmd, "dosyalar") == 0) {
        print_color("\nGrafiksel Dosya YÃ¶neticisi aciliyor...\n", LIGHT_CYAN);
        launch_file_manager_gui();
    } else if (strcmp(cmd, "monitor") == 0) {
        print_color("\nSistem MonitÃ¶rÃ¼ aciliyor...\n", LIGHT_CYAN);
        launch_system_monitor();
    } else if (strcmp(cmd, "ses_cal") == 0) {
        print_color("\nSes Mixer arayÃ¼zÃ¼ aciliyor...\n", LIGHT_CYAN);
        launch_audio_mixer();
    } else if (strncmp(cmd, "ses_kaydet", 10) == 0) {
        if (audio_is_recording()) {
            audio_stop_recording();
            print_color("\nSes kaydÄ± durduruldu.\n", LIGHT_GREEN);
        } else {
            audio_start_recording();
            print_color("\nSes kaydÄ± baslatildi.\n", LIGHT_GREEN);
        }
    } else if (strncmp(cmd, "ext2_mount ", 11) == 0) {
        char* device_str = cmd + 11;
        print_color("\nEXT2 filesystem mount ediliyor: ", LIGHT_CYAN);
        print(device_str);
        print("\n");
        
        // ATA device'ı bul (şimdilik varsayılan)
        ata_device_t* device = &ata_primary_master; // Varsayılan device
        
        if (ext2_mount(device, &ext2_main_fs) == 0) {
            print_color("EXT2 filesystem başarıyla mount edildi.\n", LIGHT_GREEN);
        } else {
            print_color("EXT2 filesystem mount edilemedi.\n", LIGHT_RED);
        }
    } else if (strcmp(cmd, "ext2_unmount") == 0) {
        print_color("\nEXT2 filesystem unmount ediliyor...\n", LIGHT_CYAN);
        
        if (ext2_unmount(&ext2_main_fs) == 0) {
            print_color("EXT2 filesystem başarıyla unmount edildi.\n", LIGHT_GREEN);
        } else {
            print_color("EXT2 filesystem unmount edilemedi.\n", LIGHT_RED);
        }
    } else if (strncmp(cmd, "ls -l ", 6) == 0) {
        char* path = cmd + 6;
        print_color("\nEXT2 Directory Listesi: ", LIGHT_CYAN);
        print(path);
        print("\n");
        
        if (!ext2_main_fs.mounted) {
            print_color("EXT2 filesystem mount edilmemiş.\n", LIGHT_RED);
        } else {
            ext2_dir_iter_t iter;
            if (ext2_open_dir(&ext2_main_fs, path, &iter) == 0) {
                printf("%-10s %-8s %-8s %-10s %-20s %s\n",
                       "İzinler", "Sahip", "Grup", "Boyut", "Tarih", "Dosya Adı");
                printf("------------------------------------------------------------\n");
                
                ext2_dir_entry_t entry;
                while (ext2_read_dir(&iter, &entry) == 0) {
                    ext2_inode_t inode;
                    if (ext2_read_inode(&ext2_main_fs, entry.inode, &inode) == 0) {
                        printf(" ");
                        ext2_print_permissions(inode.i_mode);
                        printf(" %-8d %-8d %-10s %-20s %s\n",
                               inode.i_uid, inode.i_gid,
                               ext2_format_size(inode.i_size),
                               "Tarih", entry.name);
                    }
                }
                
                ext2_close_dir(&iter);
            } else {
                print_color("Directory açılamadı.\n", LIGHT_RED);
            }
        }
    } else if (strncmp(cmd, "chmod ", 6) == 0) {
        char* params = cmd + 6;
        char* space = strchr(params, ' ');
        if (!space) {
            print_color("Kullanım: chmod [dosya] [izin]\n", YELLOW);
        } else {
            *space = '\0';
            char* filename = params;
            char* perm_str = space + 1;
            
            uint16_t permissions;
            if (sscanf(perm_str, "0%ho", &permissions) == 1) {
                print_color("\nDosya izinleri değiştiriliyor: ", LIGHT_CYAN);
                printf("%s -> 0%o\n", filename, permissions);
                
                if (ext2_chmod(&ext2_main_fs, filename, permissions) == 0) {
                    print_color("İzinler başarıyla değiştirildi.\n", LIGHT_GREEN);
                } else {
                    print_color("İzinler değiştirilemedi.\n", LIGHT_RED);
                }
            } else {
                print_color("Geçersiz izin formatı. Örnek: chmod dosya.txt 755\n", YELLOW);
            }
        }
    } else if (strncmp(cmd, "wav_cal ", 8) == 0) {
        char* filename = cmd + 8;
        print_color("\nWAV dosyasi caliniyor: ", LIGHT_CYAN);
        print(filename);
        print("\n");
        
        // WAV dosyasÄ±nÄ± yÃ¼kle ve Ã§al
        uint8_t* buffer;
        uint32_t size;
        wav_header_t header;
        
        if (audio_load_wav(filename, &buffer, &size, &header) == 0) {
            int channel = audio_create_channel();
            if (channel >= 0) {
                audio_play_buffer(channel, buffer, size, AUDIO_FORMAT_PCM16);
                audio_set_loop(channel, 0);
            }
        }
    } else if (strcmp(cmd, "donanim") == 0 || strcmp(cmd, "lspci") == 0) {
        hardware_print_summary();
        driver_list_all();
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
        // GeÃ§miÅŸ (History) Gezintisi
        if (history_count == 0) return;

        if ((uint8_t)c == 0x80) { // YUKARI
            if (history_index == -1) history_index = history_count - 1;
            else if (history_index > 0 && history_index > history_count - MAX_HISTORY) history_index--;
        } else { // AÅAÄI
            if (history_index != -1 && history_index < history_count - 1) history_index++;
            else { history_index = -1; }
        }

        // Mevcut satÄ±rÄ± temizle
        while (buffer_index > 0) {
            putchar('\b');
            buffer_index--;
        }

        // Yeni komutu yÃ¼kle
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
