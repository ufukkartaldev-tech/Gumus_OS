#include "ftp.h"
#include "tcp.h"
#include "ip.h"
#include "string.h"
#include "memory.h"
#include "stdio.h"
#include "stdlib.h"
#include "printf.h"
#include "fs.h"

// Global FTP connection
ftp_connection_t ftp_main_connection;
static uint32_t ftp_timer = 0;

// FTP error messages
static const char* ftp_error_messages[] = {
    "Başarılı",
    "Bağlantı hatası",
    "Timeout",
    "Kimlik doğrulama hatası",
    "Dosya bulunamadı",
    "Transfer hatası",
    "Komut hatası",
    "Bellek yetersiz"
};

// FTP'yi başlat
int ftp_init() {
    printf("FTP Client başlatılıyor...\n");
    
    // Main connection'ı sıfırla
    memset(&ftp_main_connection, 0, sizeof(ftp_connection_t));
    ftp_main_connection.state = FTP_DISCONNECTED;
    ftp_main_connection.control_port = FTP_CONTROL_PORT;
    
    printf("FTP Client hazır.\n");
    return 0;
}

// FTP server'ına bağlan
int ftp_connect(ftp_connection_t* conn, uint8_t* server_ip, uint16_t port) {
    if (!conn || !server_ip) {
        return -1;
    }
    
    printf("FTP Sunucusuna bağlanılıyor: %d.%d.%d.%d:%d\n", 
           server_ip[0], server_ip[1], server_ip[2], server_ip[3], port);
    
    // Connection parametrelerini ayarla
    memcpy(conn->server_ip, server_ip, 4);
    conn->control_port = port;
    conn->state = FTP_CONNECTING;
    conn->timeout = ftp_timer + FTP_TIMEOUT;
    conn->last_activity = ftp_timer;
    
    // Control connection'ı kur (TCP socket)
    conn->control_socket = tcp_connect(server_ip, port);
    if (conn->control_socket < 0) {
        printf("FTP control connection kurulamadı.\n");
        conn->state = FTP_ERROR;
        return -2;
    }
    
    // Server response'ını bekle
    if (ftp_read_response(conn) < 0) {
        printf("FTP server'dan cevap alınamadı.\n");
        conn->state = FTP_ERROR;
        return -3;
    }
    
    // 220 Ready response kontrolü
    int response_code = atoi(conn->response);
    if (response_code != FTP_READY) {
        printf("FTP server hazır değil: %s\n", conn->response);
        conn->state = FTP_ERROR;
        return -4;
    }
    
    conn->state = FTP_CONNECTED;
    strcpy(conn->current_dir, "/");
    
    printf("FTP bağlantısı kuruldu.\n");
    return 0;
}

// FTP login
int ftp_login(ftp_connection_t* conn, const char* username, const char* password) {
    if (!conn || !username || !password) {
        return -1;
    }
    
    if (conn->state != FTP_CONNECTED) {
        printf("FTP bağlantısı kurulmamış.\n");
        return -2;
    }
    
    // Kullanıcı adını gönder
    char user_cmd[128];
    sprintf(user_cmd, "USER %s\r\n", username);
    
    if (ftp_send_command(conn, user_cmd) < 0) {
        return -3;
    }
    
    if (ftp_read_response(conn) < 0) {
        return -4;
    }
    
    int response_code = atoi(conn->response);
    if (response_code != FTP_PASSWORD_NEEDED && response_code != FTP_LOGIN_OK) {
        printf("Kullanıcı adı kabul edilmedi: %s\n", conn->response);
        return -5;
    }
    
    // Parolayı gönder (gerekirse)
    if (response_code == FTP_PASSWORD_NEEDED) {
        char pass_cmd[128];
        sprintf(pass_cmd, "PASS %s\r\n", password);
        
        if (ftp_send_command(conn, pass_cmd) < 0) {
            return -6;
        }
        
        if (ftp_read_response(conn) < 0) {
            return -7;
        }
        
        response_code = atoi(conn->response);
        if (response_code != FTP_LOGIN_OK) {
            printf("Parola kabul edilmedi: %s\n", conn->response);
            return -8;
        }
    }
    
    strcpy(conn->username, username);
    strcpy(conn->password, password);
    conn->state = FTP_AUTHENTICATED;
    
    printf("FTP login başarılı.\n");
    return 0;
}

// FTP komutu gönder
int ftp_send_command(ftp_connection_t* conn, const char* command) {
    if (!conn || !command) {
        return -1;
    }
    
    if (conn->state == FTP_DISCONNECTED || conn->control_socket < 0) {
        return -2;
    }
    
    printf("FTP> %s", command);
    
    // Komutu gönder
    int len = strlen(command);
    int sent = tcp_send(conn->control_socket, (uint8_t*)command, len);
    
    if (sent != len) {
        printf("FTP komutu gönderilemedi.\n");
        return -3;
    }
    
    conn->last_activity = ftp_timer;
    return 0;
}

// FTP response oku
int ftp_read_response(ftp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    if (conn->state == FTP_DISCONNECTED || conn->control_socket < 0) {
        return -2;
    }
    
    // Response buffer'ını temizle
    memset(conn->response, 0, FTP_MAX_RESPONSE);
    conn->response_pos = 0;
    
    // Response'u oku (basit implementasyon)
    uint32_t timeout = ftp_timer + 10; // 10 saniye timeout
    
    while (ftp_timer < timeout) {
        int received = tcp_receive(conn->control_socket, 
                                   (uint8_t*)&conn->response[conn->response_pos],
                                   FTP_MAX_RESPONSE - conn->response_pos - 1);
        
        if (received > 0) {
            conn->response_pos += received;
            conn->response[conn->response_pos] = '\0';
            
            // Response tamamlandı mı kontrol et (newline ile bitiyor mu)
            if (conn->response_pos > 0 && 
                (conn->response[conn->response_pos-1] == '\n' ||
                 conn->response[conn->response_pos-1] == '\r')) {
                printf("FTP< %s", conn->response);
                conn->last_activity = ftp_timer;
                return 0;
            }
        }
        
        // Küçük bekleme
        ftp_tick(conn);
    }
    
    printf("FTP response timeout.\n");
    return -3;
}

// Belirli bir response kodunu bekle
int ftp_wait_response(ftp_connection_t* conn, int expected_code) {
    if (ftp_read_response(conn) < 0) {
        return -1;
    }
    
    int response_code = atoi(conn->response);
    if (response_code != expected_code) {
        printf("Beklenmeyen FTP response: %d (beklenen: %d)\n", 
               response_code, expected_code);
        return -2;
    }
    
    return 0;
}

// Passive mode'a geç
int ftp_enter_passive_mode(ftp_connection_t* conn, uint8_t* data_ip, uint16_t* data_port) {
    if (!conn || !data_ip || !data_port) {
        return -1;
    }
    
    // PASV komutu gönder
    if (ftp_send_command(conn, "PASV\r\n") < 0) {
        return -2;
    }
    
    if (ftp_read_response(conn) < 0) {
        return -3;
    }
    
    // Response'u parse et
    if (ftp_parse_pasv_response(conn->response, data_ip, data_port) < 0) {
        printf("PASV response parse edilemedi: %s\n", conn->response);
        return -4;
    }
    
    conn->mode = FTP_PASSIVE;
    printf("Passive mode: %d.%d.%d.%d:%d\n", 
           data_ip[0], data_ip[1], data_ip[2], data_ip[3], *data_port);
    
    return 0;
}

// PASV response'unu parse et
int ftp_parse_pasv_response(const char* response, uint8_t* ip, uint16_t* port) {
    // Format: 227 Entering Passive Mode (192,168,1,100,200,5)
    const char* start = strchr(response, '(');
    if (!start) {
        return -1;
    }
    
    start++;
    int a, b, c, d, p1, p2;
    if (sscanf(start, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &p1, &p2) != 6) {
        return -2;
    }
    
    ip[0] = a;
    ip[1] = b;
    ip[2] = c;
    ip[3] = d;
    *port = (p1 << 8) | p2;
    
    return 0;
}

// Data connection aç
int ftp_open_data_connection(ftp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    uint8_t data_ip[4];
    uint16_t data_port;
    
    if (conn->mode == FTP_PASSIVE) {
        // Passive mode
        if (ftp_enter_passive_mode(conn, data_ip, &data_port) < 0) {
            return -2;
        }
    } else {
        // Active mode (şimdilik implemente edilmedi)
        printf("Active mode henüz desteklenmiyor.\n");
        return -3;
    }
    
    // Data connection'ı kur
    conn->data_socket = tcp_connect(data_ip, data_port);
    if (conn->data_socket < 0) {
        printf("Data connection kurulamadı.\n");
        return -4;
    }
    
    conn->state = FTP_TRANSFER_READY;
    return 0;
}

// Directory listele
int ftp_list_directory(ftp_connection_t* conn, ftp_directory_t* dir) {
    if (!conn || !dir) {
        return -1;
    }
    
    if (conn->state != FTP_AUTHENTICATED && conn->state != FTP_TRANSFER_READY) {
        return -2;
    }
    
    // Data connection aç
    if (ftp_open_data_connection(conn) < 0) {
        return -3;
    }
    
    // LIST komutu gönder
    if (ftp_send_command(conn, "LIST\r\n") < 0) {
        return -4;
    }
    
    // File transfer response'unu bekle
    if (ftp_wait_response(conn, FTP_FILE_OK) < 0) {
        return -5;
    }
    
    // Data connection'dan veri oku
    memset(dir, 0, sizeof(ftp_directory_t));
    char line_buffer[512];
    int line_pos = 0;
    int entry_count = 0;
    
    uint32_t timeout = ftp_timer + 30; // 30 saniye timeout
    
    while (ftp_timer < timeout && entry_count < 256) {
        int received = tcp_receive(conn->data_socket, 
                                   (uint8_t*)&line_buffer[line_pos],
                                   sizeof(line_buffer) - line_pos - 1);
        
        if (received > 0) {
            line_pos += received;
            line_buffer[line_pos] = '\0';
            
            // Satır satır parse et
            char* line_start = line_buffer;
            char* newline;
            
            while ((newline = strchr(line_start, '\n')) != NULL) {
                *newline = '\0';
                
                // Satır sonundaki \r'i temizle
                char* cr = strchr(line_start, '\r');
                if (cr) *cr = '\0';
                
                // Satırı parse et
                if (strlen(line_start) > 0) {
                    if (ftp_parse_list_line(line_start, &dir->entries[entry_count]) == 0) {
                        entry_count++;
                    }
                }
                
                line_start = newline + 1;
            }
            
            // Kalan veriyi buffer başına taşı
            if (line_start < line_buffer + line_pos) {
                int remaining = line_buffer + line_pos - line_start;
                memmove(line_buffer, line_start, remaining);
                line_pos = remaining;
            } else {
                line_pos = 0;
            }
        }
        
        ftp_tick(conn);
    }
    
    dir->count = entry_count;
    
    // Data connection'ı kapat
    tcp_close(conn->data_socket);
    conn->data_socket = -1;
    
    // Transfer complete response'unu bekle
    if (ftp_wait_response(conn, FTP_TRANSFER_OK) < 0) {
        return -6;
    }
    
    printf("%d dosya/dizin bulundu.\n", entry_count);
    return 0;
}

// LIST satırını parse et
int ftp_parse_list_line(const char* line, ftp_file_entry_t* entry) {
    if (!line || !entry) {
        return -1;
    }
    
    // Unix format: -rw-r--r-- 1 owner group size date filename
    char permissions[16], owner[16], group[16], date[32], filename[FTP_MAX_FILENAME];
    uint32_t size;
    
    int parsed = sscanf(line, "%15s %15s %15s %u %31s %255s",
                       permissions, owner, group, &size, date, filename);
    
    if (parsed < 6) {
        return -2;
    }
    
    strcpy(entry->permissions, permissions);
    strcpy(entry->owner, owner);
    strcpy(entry->group, group);
    strcpy(entry->date, date);
    strcpy(entry->name, filename);
    entry->size = size;
    
    // Dosya türünü belirle
    if (strlen(permissions) > 0) {
        entry->type = permissions[0];
    } else {
        entry->type = '-';
    }
    
    return 0;
}

// Directory yazdır
void ftp_print_directory(ftp_directory_t* dir) {
    if (!dir) {
        return;
    }
    
    printf("\n=== FTP Directory Listing ===\n");
    printf("%-10s %-8s %-8s %-10s %-12s %s\n", 
           "İzinler", "Sahip", "Grup", "Boyut", "Tarih", "Dosya Adı");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < dir->count; i++) {
        ftp_file_entry_t* entry = &dir->entries[i];
        
        char type_char = entry->type;
        if (type_char == 'd') {
            printf_color(LIGHT_BLUE, "%c%-9s ", type_char, entry->permissions + 1);
        } else if (type_char == 'l') {
            printf_color(LIGHT_CYAN, "%c%-9s ", type_char, entry->permissions + 1);
        } else {
            printf("%c%-9s ", type_char, entry->permissions + 1);
        }
        
        printf("%-8s %-8s %-10u %-12s %s\n",
               entry->owner, entry->group, entry->size, entry->date, entry->name);
    }
    
    printf("Toplam: %d dosya/dizin\n", dir->count);
    printf("=============================\n");
}

// Dosya indir
int ftp_download_file(ftp_connection_t* conn, const char* remote_file, const char* local_file) {
    if (!conn || !remote_file || !local_file) {
        return -1;
    }
    
    printf("Dosya indiriliyor: %s -> %s\n", remote_file, local_file);
    
    // Data connection aç
    if (ftp_open_data_connection(conn) < 0) {
        return -2;
    }
    
    // RETR komutu gönder
    char retr_cmd[512];
    sprintf(retr_cmd, "RETR %s\r\n", remote_file);
    
    if (ftp_send_command(conn, retr_cmd) < 0) {
        return -3;
    }
    
    // File transfer response'unu bekle
    if (ftp_wait_response(conn, FTP_FILE_OK) < 0) {
        return -4;
    }
    
    // Dosyayı oluştur
    FILE* file = fopen(local_file, "wb");
    if (!file) {
        printf("Local dosya oluşturulamadı: %s\n", local_file);
        tcp_close(conn->data_socket);
        return -5;
    }
    
    // Veriyi indir
    uint32_t total_received = 0;
    uint8_t buffer[FTP_BUFFER_SIZE];
    uint32_t timeout = ftp_timer + 300; // 5 dakika timeout
    
    while (ftp_timer < timeout) {
        int received = tcp_receive(conn->data_socket, buffer, sizeof(buffer));
        
        if (received > 0) {
            fwrite(buffer, 1, received, file);
            total_received += received;
            conn->bytes_transferred = total_received;
            
            // Progress göster
            if (total_received % 10240 == 0) { // Her 10KB
                printf("\rİndiriliyor: %u bytes", total_received);
            }
        } else if (received == 0) {
            // Transfer tamamlandı
            break;
        }
        
        ftp_tick(conn);
    }
    
    fclose(file);
    tcp_close(conn->data_socket);
    conn->data_socket = -1;
    
    // Transfer complete response'unu bekle
    if (ftp_wait_response(conn, FTP_TRANSFER_OK) < 0) {
        return -6;
    }
    
    printf("\nDosya başarıyla indirildi: %u bytes\n", total_received);
    return 0;
}

// FTP bağlantısını kapat
int ftp_disconnect(ftp_connection_t* conn) {
    if (!conn) {
        return -1;
    }
    
    if (conn->state != FTP_DISCONNECTED) {
        // QUIT komutu gönder
        ftp_send_command(conn, "QUIT\r\n");
        ftp_read_response(conn);
        
        // Connection'ları kapat
        if (conn->control_socket >= 0) {
            tcp_close(conn->control_socket);
            conn->control_socket = -1;
        }
        
        if (conn->data_socket >= 0) {
            tcp_close(conn->data_socket);
            conn->data_socket = -1;
        }
    }
    
    conn->state = FTP_DISCONNECTED;
    printf("FTP bağlantısı kapatıldı.\n");
    return 0;
}

// FTP connection durumunu kontrol et
int ftp_is_connected(ftp_connection_t* conn) {
    if (!conn) {
        return 0;
    }
    
    return (conn->state == FTP_CONNECTED || 
            conn->state == FTP_AUTHENTICATED || 
            conn->state == FTP_TRANSFER_READY);
}

// FTP tick (timer ve timeout kontrolü)
void ftp_tick(ftp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    ftp_timer++;
    
    // Timeout kontrolü
    if (conn->state != FTP_DISCONNECTED && 
        ftp_timer > conn->timeout) {
        printf("FTP connection timeout.\n");
        conn->state = FTP_ERROR;
    }
}

// Error string'i al
const char* ftp_get_error_string(int error_code) {
    if (error_code >= 0 && error_code < 8) {
        return ftp_error_messages[error_code];
    }
    return "Bilinmeyen hata";
}
