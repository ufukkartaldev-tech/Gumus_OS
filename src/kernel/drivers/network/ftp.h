#ifndef FTP_H
#define FTP_H

#include <stdint.h>
#include <stddef.h>
#include "ip.h"
#include "tcp.h"

// FTP Sabitleri
#define FTP_CONTROL_PORT        21
#define FTP_DATA_PORT           20
#define FTP_MAX_RESPONSE        1024
#define FTP_MAX_FILENAME        256
#define FTP_MAX_PATH           512
#define FTP_BUFFER_SIZE        4096
#define FTP_TIMEOUT            30

// FTP Response Codes
#define FTP_READY              220
#define FTP_PASSWORD_NEEDED    331
#define FTP_LOGIN_OK           230
#define FTP_FILE_OK            150
#define FTP_TRANSFER_OK        226
#define FTP_PASSIVE_MODE       227
#define FTP_DIRECTORY_OK       250
#define FTP_FILE_ACTION_OK     250
#define FTP_COMMAND_OK         200
#define FTP_NOT_FOUND          550
#define FTP_ERROR              500

// FTP Transfer Modes
#define FTP_MODE_ASCII         'A'
#define FTP_MODE_BINARY        'I'

// FTP Connection Types
typedef enum {
    FTP_ACTIVE,
    FTP_PASSIVE
} ftp_mode_t;

// FTP Connection States
typedef enum {
    FTP_DISCONNECTED,
    FTP_CONNECTING,
    FTP_CONNECTED,
    FTP_AUTHENTICATED,
    FTP_TRANSFER_READY,
    FTP_TRANSFERRING,
    FTP_ERROR
} ftp_state_t;

// FTP Transfer Types
typedef enum {
    FTP_TRANSFER_NONE,
    FTP_TRANSFER_DOWNLOAD,
    FTP_TRANSFER_UPLOAD,
    FTP_TRANSFER_LIST
} ftp_transfer_type_t;

// FTP Connection Structure
typedef struct {
    uint8_t server_ip[4];          // FTP server IP adresi
    uint16_t control_port;        // Control port (genellikle 21)
    uint16_t data_port;           // Data port için
    int control_socket;           // Control connection socket
    int data_socket;               // Data connection socket
    
    char username[64];             // Kullanıcı adı
    char password[64];             // Parola
    char current_dir[FTP_MAX_PATH]; // Mevcut dizin
    
    ftp_state_t state;            // Connection durumu
    ftp_mode_t mode;               // FTP modu (active/passive)
    ftp_transfer_type_t transfer; // Transfer türü
    
    uint32_t timeout;             // Timeout zamanı
    uint32_t last_activity;       // Son aktivite zamanı
    
    char response[FTP_MAX_RESPONSE]; // Son response buffer'ı
    int response_pos;             // Response pozisyonu
    
    uint8_t buffer[FTP_BUFFER_SIZE]; // Transfer buffer'ı
    uint32_t buffer_size;         // Buffer boyutu
    uint32_t bytes_transferred;   // Transfer edilen byte sayısı
    
} ftp_connection_t;

// FTP File Entry (LIST komutu için)
typedef struct {
    char name[FTP_MAX_FILENAME];
    char permissions[16];
    char owner[16];
    char group[16];
    uint32_t size;
    char date[32];
    char type;                    // 'd' = directory, '-' = file, 'l' = link
} ftp_file_entry_t;

// FTP Directory Listing
typedef struct {
    ftp_file_entry_t entries[256];
    int count;
} ftp_directory_t;

// FTP Fonksiyonları
int ftp_init();
int ftp_connect(ftp_connection_t* conn, uint8_t* server_ip, uint16_t port);
int ftp_login(ftp_connection_t* conn, const char* username, const char* password);
int ftp_disconnect(ftp_connection_t* conn);
int ftp_set_mode(ftp_connection_t* conn, ftp_mode_t mode);
int ftp_set_transfer_type(ftp_connection_t* conn, char type);

// FTP Komutları
int ftp_send_command(ftp_connection_t* conn, const char* command);
int ftp_read_response(ftp_connection_t* conn);
int ftp_wait_response(ftp_connection_t* conn, int expected_code);

// Directory İşlemleri
int ftp_change_directory(ftp_connection_t* conn, const char* path);
int ftp_print_working_directory(ftp_connection_t* conn, char* path);
int ftp_list_directory(ftp_connection_t* conn, ftp_directory_t* dir);
int ftp_create_directory(ftp_connection_t* conn, const char* name);
int ftp_remove_directory(ftp_connection_t* conn, const char* name);

// File Transfer İşlemleri
int ftp_download_file(ftp_connection_t* conn, const char* remote_file, const char* local_file);
int ftp_upload_file(ftp_connection_t* conn, const char* local_file, const char* remote_file);
int ftp_delete_file(ftp_connection_t* conn, const char* filename);
int ftp_rename_file(ftp_connection_t* conn, const char* old_name, const char* new_name);
int ftp_get_file_size(ftp_connection_t* conn, const char* filename, uint32_t* size);

// Passive Mode İşlemleri
int ftp_enter_passive_mode(ftp_connection_t* conn, uint8_t* data_ip, uint16_t* data_port);
int ftp_open_data_connection(ftp_connection_t* conn);

// Active Mode İşlemleri
int ftp_setup_active_mode(ftp_connection_t* conn, uint16_t port);

// Utility Fonksiyonları
int ftp_parse_pasv_response(const char* response, uint8_t* ip, uint16_t* port);
int ftp_parse_list_line(const char* line, ftp_file_entry_t* entry);
int ftp_parse_file_size(const char* response, uint32_t* size);
void ftp_print_directory(ftp_directory_t* dir);
int ftp_is_connected(ftp_connection_t* conn);
void ftp_tick(ftp_connection_t* conn);

// Error Handling
const char* ftp_get_error_string(int error_code);
int ftp_get_last_error(ftp_connection_t* conn);

// FTP Global Variables
extern ftp_connection_t ftp_main_connection;

#endif
