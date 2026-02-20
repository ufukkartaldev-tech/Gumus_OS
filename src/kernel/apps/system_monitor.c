#include "system_monitor.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "memory.h"
#include "string.h"
#include "fs.h"
#include "ata.h"
#include "../drivers/mouse.h"
#include "../cpu/task.h"

// Global değişkenler
static sm_data_t* sm_data = NULL;
static int sm_initialized = 0;
static uint32_t system_start_time = 0;

// CPU kullanımını ölçmek için sayaçlar
static uint32_t cpu_idle_counter = 0;
static uint32_t cpu_total_counter = 0;
static uint32_t last_cpu_check = 0;

void sm_init() {
    if (sm_initialized) return;
    
    sm_data = (sm_data_t*)kmalloc(sizeof(sm_data_t));
    if (!sm_data) return;
    
    memset(sm_data, 0, sizeof(sm_data_t));
    sm_data->refresh_rate = 10; // Her 10 tick'te bir güncelle
    sm_data->show_graphs = 1;
    sm_data->show_details = 1;
    
    system_start_time = get_tick_count(); // Sistem başlangıç zamanı
    
    sm_initialized = 1;
    printf("Sistem Monitörü başlatıldı\n");
}

uint32_t sm_get_uptime() {
    if (!sm_initialized) return 0;
    return get_tick_count() - system_start_time;
}

void sm_update_cpu_stats() {
    if (!sm_data) return;
    
    uint32_t current_time = get_tick_count();
    uint32_t time_diff = current_time - last_cpu_check;
    
    if (time_diff < 100) return; // 100 tick'ten sık güncelleme
    
    // Basit CPU kullanım hesaplaması
    // Gerçek bir sistemde bu CPU kesmelerinden ölçülür
    uint32_t current_total = current_time;
    uint32_t current_idle = cpu_idle_counter;
    
    uint32_t total_diff = current_total - sm_data->cpu.total_ticks;
    uint32_t idle_diff = current_idle - sm_data->cpu.idle_ticks;
    
    if (total_diff > 0) {
        sm_data->cpu.usage_percent = 100 - ((idle_diff * 100) / total_diff);
        if (sm_data->cpu.usage_percent > 100) sm_data->cpu.usage_percent = 100;
    }
    
    sm_data->cpu.total_ticks = current_total;
    sm_data->cpu.idle_ticks = current_idle;
    
    // Geçmişe ekle
    sm_data->cpu.history[sm_data->cpu.history_index] = sm_data->cpu.usage_percent;
    sm_data->cpu.history_index = (sm_data->cpu.history_index + 1) % SM_MAX_HISTORY;
    
    last_cpu_check = current_time;
}

void sm_update_ram_stats() {
    if (!sm_data) return;
    
    // Bellek bilgilerini al
    extern uint32_t used_blocks;
    extern uint32_t max_blocks;
    
    uint32_t block_size = 4096; // 4KB sayfa boyutu
    sm_data->ram.total_memory = max_blocks * block_size;
    sm_data->ram.used_memory = used_blocks * block_size;
    sm_data->ram.free_memory = sm_data->ram.total_memory - sm_data->ram.used_memory;
    
    if (sm_data->ram.total_memory > 0) {
        sm_data->ram.usage_percent = (sm_data->ram.used_memory * 100) / sm_data->ram.total_memory;
    }
    
    // Kernel ve user bellek ayrımı (basit tahmin)
    sm_data->ram.kernel_memory = sm_data->ram.used_memory / 3;
    sm_data->ram.user_memory = sm_data->ram.used_memory - sm_data->ram.kernel_memory;
    
    // Geçmişe ekle
    sm_data->ram.history[sm_data->ram.history_index] = sm_data->ram.usage_percent;
    sm_data->ram.history_index = (sm_data->ram.history_index + 1) % SM_MAX_HISTORY;
}

void sm_update_system_info() {
    if (!sm_data) return;
    
    sm_data->system.uptime_seconds = sm_get_uptime() / 100; // Tick'i saniyeye çevir
    
    // Process sayısı (basit tahmin)
    sm_data->system.total_processes = 5; // Sabit değer
    sm_data->system.active_processes = 3; // Sabit değer
    
    // Interrupt sayısı (basit tahmin)
    static uint32_t interrupt_counter = 0;
    interrupt_counter += 10; // Her güncellemede 10 interrupt varsayalım
    sm_data->system.total_interrupts = interrupt_counter;
    
    // Context switch sayısı (basit tahmin)
    static uint32_t context_switch_counter = 0;
    context_switch_counter += 5; // Her güncellemede 5 switch varsayalım
    sm_data->system.context_switches = context_switch_counter;
}

void sm_update_driver_stats() {
    if (!sm_data) return;
    
    // Disk istatistikleri
    static uint32_t disk_read_counter = 0;
    static uint32_t disk_write_counter = 0;
    
    disk_read_counter += 2; // Simüle edilmiş disk okuma
    disk_write_counter += 1; // Simüle edilmiş disk yazma
    
    sm_data->drivers.disk_reads = disk_read_counter;
    sm_data->drivers.disk_writes = disk_write_counter;
    
    // Network istatistikleri
    static uint32_t network_sent_counter = 0;
    static uint32_t network_recv_counter = 0;
    
    network_sent_counter += 3; // Simüle edilmiş network gönderme
    network_recv_counter += 4; // Simüle edilmiş network alma
    
    sm_data->drivers.network_packets_sent = network_sent_counter;
    sm_data->drivers.network_packets_received = network_recv_counter;
    
    // Input istatistikleri
    static uint32_t keyboard_counter = 0;
    static uint32_t mouse_counter = 0;
    
    keyboard_counter += 8; // Simüle edilmiş klavye tuşu
    mouse_counter += 2; // Simüle edilmiş mouse tıklama
    
    sm_data->drivers.keyboard_presses = keyboard_counter;
    sm_data->drivers.mouse_clicks = mouse_counter;
}

void sm_update() {
    if (!sm_initialized || !sm_data) return;
    
    sm_data->update_counter++;
    
    if (sm_data->update_counter % sm_data->refresh_rate != 0) {
        return; // Henüz güncelleme zamanı değil
    }
    
    sm_update_cpu_stats();
    sm_update_ram_stats();
    sm_update_system_info();
    sm_update_driver_stats();
    
    sm_data->last_update_time = get_tick_count();
}

void sm_format_bytes(uint32_t bytes, char* buffer) {
    if (bytes < 1024) {
        sprintf(buffer, "%d B", bytes);
    } else if (bytes < 1024 * 1024) {
        sprintf(buffer, "%d KB", bytes / 1024);
    } else if (bytes < 1024 * 1024 * 1024) {
        sprintf(buffer, "%d MB", bytes / (1024 * 1024));
    } else {
        sprintf(buffer, "%d GB", bytes / (1024 * 1024 * 1024));
    }
}

void sm_format_time(uint32_t seconds, char* buffer) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    if (hours > 0) {
        sprintf(buffer, "%dh %dm %ds", hours, minutes, secs);
    } else if (minutes > 0) {
        sprintf(buffer, "%dm %ds", minutes, secs);
    } else {
        sprintf(buffer, "%ds", secs);
    }
}

void sm_draw_bar_graph(int x, int y, int width, int height, uint32_t value, uint32_t max_value, uint8_t color) {
    // Arka plan
    vga_draw_rect(x, y, width, height, SM_BG_COLOR);
    
    // Çerçeve
    vga_draw_rect(x, y, width, height, SM_BORDER_COLOR);
    
    // Dolgu çubuğu
    if (max_value > 0 && value > 0) {
        int fill_width = (value * (width - 2)) / max_value;
        if (fill_width > width - 2) fill_width = width - 2;
        
        vga_draw_rect(x + 1, y + 1, fill_width, height - 2, color);
    }
    
    // Yüzde text'i
    char percent_text[16];
    sprintf(percent_text, "%d%%", value);
    int text_x = x + (width - strlen(percent_text) * 8) / 2;
    int text_y = y + (height - SM_TEXT_HEIGHT) / 2;
    vga_draw_text(text_x, text_y, percent_text, SM_TEXT_COLOR);
}

void sm_draw_line_graph(int x, int y, int width, int height, uint32_t* data, int data_count, uint8_t color) {
    // Arka plan
    vga_draw_rect(x, y, width, height, SM_BG_COLOR);
    
    // Çerçeve
    vga_draw_rect(x, y, width, height, SM_BORDER_COLOR);
    
    // Grid
    sm_draw_grid(x, y, width, height, 10);
    
    // Veri çizgisi
    if (data_count > 1) {
        for (int i = 1; i < data_count; i++) {
            int x1 = x + 1 + ((i - 1) * (width - 2)) / (data_count - 1);
            int y1 = y + height - 1 - (data[i - 1] * (height - 2)) / 100;
            int x2 = x + 1 + (i * (width - 2)) / (data_count - 1);
            int y2 = y + height - 1 - (data[i] * (height - 2)) / 100;
            
            vga_draw_line(x1, y1, x2, y2, color);
        }
    }
}

void sm_draw_grid(int x, int y, int width, int height, int grid_size) {
    // Yatay grid çizgileri
    for (int i = 1; i < 5; i++) {
        int grid_y = y + (i * height) / 5;
        vga_draw_line(x, grid_y, x + width, grid_y, SM_GRID_COLOR);
    }
    
    // Dikey grid çizgileri
    for (int i = 1; i < grid_size; i++) {
        int grid_x = x + (i * width) / grid_size;
        vga_draw_line(grid_x, y, grid_x, y + height, SM_GRID_COLOR);
    }
}

void sm_draw_cpu_section(window_t* win, int x, int y, int width, int height) {
    if (!sm_data) return;
    
    // Başlık
    vga_draw_text(x, y, "CPU Kullanımı", SM_HEADER_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Bar grafik
    sm_draw_bar_graph(x, y, width - 120, SM_BAR_HEIGHT, 
                      sm_data->cpu.usage_percent, 100, SM_CPU_COLOR);
    
    // Detay text'i
    char detail_text[64];
    sprintf(detail_text, "User: %d%% Kernel: %d%% Idle: %d%%", 
            sm_data->cpu.user_ticks, sm_data->cpu.kernel_ticks, 
            100 - sm_data->cpu.usage_percent);
    vga_draw_text(x + width - 110, y + 4, detail_text, SM_TEXT_COLOR);
    
    y += SM_BAR_HEIGHT + SM_SECTION_SPACING;
    
    // Çizgi grafik
    if (sm_data->show_graphs) {
        sm_draw_line_graph(x, y, width, SM_GRAPH_HEIGHT, 
                           sm_data->cpu.history, SM_MAX_HISTORY, SM_CPU_COLOR);
    }
}

void sm_draw_ram_section(window_t* win, int x, int y, int width, int height) {
    if (!sm_data) return;
    
    // Başlık
    vga_draw_text(x, y, "Bellek Kullanımı", SM_HEADER_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Bar grafik
    sm_draw_bar_graph(x, y, width - 120, SM_BAR_HEIGHT, 
                      sm_data->ram.usage_percent, 100, SM_RAM_COLOR);
    
    // Detay text'i
    char used_text[32], total_text[32];
    sm_format_bytes(sm_data->ram.used_memory, used_text);
    sm_format_bytes(sm_data->ram.total_memory, total_text);
    
    char detail_text[64];
    sprintf(detail_text, "%s / %s", used_text, total_text);
    vga_draw_text(x + width - 110, y + 4, detail_text, SM_TEXT_COLOR);
    
    y += SM_BAR_HEIGHT + SM_SECTION_SPACING;
    
    // Çizgi grafik
    if (sm_data->show_graphs) {
        sm_draw_line_graph(x, y, width, SM_GRAPH_HEIGHT, 
                           sm_data->ram.history, SM_MAX_HISTORY, SM_RAM_COLOR);
    }
}

void sm_draw_system_info_section(window_t* win, int x, int y, int width, int height) {
    if (!sm_data) return;
    
    // Başlık
    vga_draw_text(x, y, "Sistem Bilgileri", SM_HEADER_COLOR);
    y += SM_TEXT_HEIGHT + 8;
    
    // Uptime
    char uptime_text[32];
    sm_format_time(sm_data->system.uptime_seconds, uptime_text);
    vga_draw_text(x, y, "Uptime: ", SM_TEXT_COLOR);
    vga_draw_text(x + 60, y, uptime_text, SM_TEXT_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Process bilgileri
    char process_text[32];
    sprintf(process_text, "Processes: %d/%d", sm_data->system.active_processes, 
            sm_data->system.total_processes);
    vga_draw_text(x, y, process_text, SM_TEXT_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Interrupt bilgileri
    char interrupt_text[32];
    sprintf(interrupt_text, "Interrupts: %d", sm_data->system.total_interrupts);
    vga_draw_text(x, y, interrupt_text, SM_TEXT_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Context switch bilgileri
    char context_text[32];
    sprintf(context_text, "Context Switches: %d", sm_data->system.context_switches);
    vga_draw_text(x, y, context_text, SM_TEXT_COLOR);
}

void sm_draw_driver_stats_section(window_t* win, int x, int y, int width, int height) {
    if (!sm_data) return;
    
    // Başlık
    vga_draw_text(x, y, "Sürücü İstatistikleri", SM_HEADER_COLOR);
    y += SM_TEXT_HEIGHT + 8;
    
    // Disk istatistikleri
    char disk_text[32];
    sprintf(disk_text, "Disk: %dR/%dW", sm_data->drivers.disk_reads, 
            sm_data->drivers.disk_writes);
    vga_draw_text(x, y, disk_text, SM_TEXT_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Network istatistikleri
    char network_text[32];
    sprintf(network_text, "Network: %dS/%dR", sm_data->drivers.network_packets_sent, 
            sm_data->drivers.network_packets_received);
    vga_draw_text(x, y, network_text, SM_TEXT_COLOR);
    y += SM_TEXT_HEIGHT + 4;
    
    // Input istatistikleri
    char input_text[32];
    sprintf(input_text, "Input: %dK/%dM", sm_data->drivers.keyboard_presses, 
            sm_data->drivers.mouse_clicks);
    vga_draw_text(x, y, input_text, SM_TEXT_COLOR);
}

void sm_draw_window(window_t* win) {
    if (!sm_initialized || !sm_data) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    int height = win->h * 8;
    
    // Ana arka plan
    vga_draw_rect(px + 1, py + 1, width - 2, height - 2, SM_BG_COLOR);
    
    // Bölüm ayırıcı çizgiler
    int section_width = width / 2;
    int section_height = height / 2;
    
    vga_draw_line(px + section_width, py + 20, px + section_width, py + height - 2, SM_BORDER_COLOR);
    vga_draw_line(px + 2, py + section_height, px + width - 2, py + section_height, SM_BORDER_COLOR);
    
    // CPU bölümü (sol üst)
    sm_draw_cpu_section(win, px + 4, py + 4, section_width - 6, section_height - 6);
    
    // RAM bölümü (sağ üst)
    sm_draw_ram_section(win, px + section_width + 4, py + 4, section_width - 6, section_height - 6);
    
    // Sistem bilgileri bölümü (sol alt)
    sm_draw_system_info_section(win, px + 4, py + section_height + 4, section_width - 6, section_height - 6);
    
    // Sürücü istatistikleri bölümü (sağ alt)
    sm_draw_driver_stats_section(win, px + section_width + 4, py + section_height + 4, section_width - 6, section_height - 6);
    
    // Son güncelleme zamanı
    char update_text[32];
    sprintf(update_text, "Update: %d", sm_data->update_counter);
    vga_draw_text(px + width - 80, py + height - 15, update_text, SM_GRID_COLOR);
}

void sm_handle_click(window_t* win, int x, int y) {
    if (!sm_data) return;
    
    // Pencere koordinatlarını dönüştür
    int px = x - win->x * 8;
    int py = y - win->y * 8;
    
    // Grafik göster/gizle toggle
    if (px > win->w * 8 - 100 && py < 20) {
        sm_data->show_graphs = !sm_data->show_graphs;
    }
    
    // Detay göster/gizle toggle
    if (px > win->w * 8 - 100 && py > 20 && py < 40) {
        sm_data->show_details = !sm_data->show_details;
    }
}

void sm_handle_key(window_t* win, char c) {
    if (!sm_data) return;
    
    switch (c) {
        case 'r':
        case 'R':
            // Manuel güncelleme
            sm_update_cpu_stats();
            sm_update_ram_stats();
            sm_update_system_info();
            sm_update_driver_stats();
            break;
        case 'g':
        case 'G':
            // Grafik göster/gizle
            sm_data->show_graphs = !sm_data->show_graphs;
            break;
        case 'd':
        case 'D':
            // Detay göster/gizle
            sm_data->show_details = !sm_data->show_details;
            break;
        case 'q':
        case 'Q':
            // Pencereyi kapat
            close_window(win->id);
            break;
    }
}

void launch_system_monitor() {
    sm_init();
    
    int win_id = create_window("Sistem Monitörü", 5, 3, 80, 25, (CYAN << 4) | BLACK);
    window_t* win = get_window(win_id);
    
    if (win) {
        win->draw_content = sm_draw_window;
        win->on_click = sm_handle_click;
        win->on_key = sm_handle_key;
        win->on_update = sm_update;
        win->data = sm_data;
    }
}
