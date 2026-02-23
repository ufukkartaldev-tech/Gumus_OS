#include "system_monitor.h"
#include "window.h"
#include "kernel.h"
#include "vga_gfx.h"
#include "memory.h"
#include "string.h"
#include "fs.h"
#include "ata.h"
#include "mouse.h"
#include "task.h"
#include "printf.h"

// Global değişkenler
static sm_data_t* sm_data = NULL;
static int sm_initialized = 0;
static uint32_t system_start_time = 0;

// CPU kullanımı için sayaçlar
static uint32_t cpu_idle_counter = 0;
static uint32_t last_cpu_check = 0;

// RAM için extern tanımlar (Kernel memory manager'dan gelir)
extern uint32_t used_blocks;
extern uint32_t max_blocks;

// Zamanlayıcı fonksiyonun (Prototipini buraya ekleyelim ki hata vermesin)
extern uint32_t get_tick_count();

void sm_init() {
    if (sm_initialized) return;
    
    sm_data = (sm_data_t*)kmalloc(sizeof(sm_data_t));
    if (!sm_data) return;
    
    memset(sm_data, 0, sizeof(sm_data_t));
    sm_data->refresh_rate = 10; 
    sm_data->show_graphs = 1;
    sm_data->show_details = 1;
    
    system_start_time = get_tick_count();
    
    sm_initialized = 1;
    printf("Sistem Monitoru baslatildi\n");
}

uint32_t sm_get_uptime() {
    if (!sm_initialized) return 0;
    return get_tick_count() - system_start_time;
}

void sm_update_cpu_stats() {
    if (!sm_data) return;
    
    uint32_t current_time = get_tick_count();
    uint32_t time_diff = current_time - last_cpu_check;
    
    if (time_diff < 100) return; 
    
    uint32_t total_diff = current_time - sm_data->cpu.total_ticks;
    uint32_t idle_diff = cpu_idle_counter - sm_data->cpu.idle_ticks;
    
    if (total_diff > 0) {
        sm_data->cpu.usage_percent = 100 - ((idle_diff * 100) / total_diff);
        if (sm_data->cpu.usage_percent > 100) sm_data->cpu.usage_percent = 100;
    }
    
    sm_data->cpu.total_ticks = current_time;
    sm_data->cpu.idle_ticks = cpu_idle_counter;
    
    sm_data->cpu.history[sm_data->cpu.history_index] = sm_data->cpu.usage_percent;
    sm_data->cpu.history_index = (sm_data->cpu.history_index + 1) % SM_MAX_HISTORY;
    
    last_cpu_check = current_time;
}

void sm_update_ram_stats() {
    if (!sm_data) return;
    
    uint32_t block_size = 4096; 
    sm_data->ram.total_memory = max_blocks * block_size;
    sm_data->ram.used_memory = used_blocks * block_size;
    sm_data->ram.free_memory = sm_data->ram.total_memory - sm_data->ram.used_memory;
    
    if (sm_data->ram.total_memory > 0) {
        sm_data->ram.usage_percent = (sm_data->ram.used_memory * 100) / sm_data->ram.total_memory;
    }
    
    sm_data->ram.history[sm_data->ram.history_index] = sm_data->ram.usage_percent;
    sm_data->ram.history_index = (sm_data->ram.history_index + 1) % SM_MAX_HISTORY;
}

void sm_update_system_info() {
    if (!sm_data) return;
    sm_data->system.uptime_seconds = sm_get_uptime() / 100;
    sm_data->system.total_processes = 5; 
    sm_data->system.active_processes = 3; 
}

void sm_update_driver_stats() {
    if (!sm_data) return;
    static uint32_t disk_read_counter = 0;
    disk_read_counter += 2; 
    sm_data->drivers.disk_reads = disk_read_counter;
}

// GÜNCELLEME: window_t* parametresi eklendi
void sm_update(window_t* win) {
    if (!sm_initialized || !sm_data) return;
    
    sm_data->update_counter++;
    if (sm_data->update_counter % sm_data->refresh_rate != 0) return;
    
    sm_update_cpu_stats();
    sm_update_ram_stats();
    sm_update_system_info();
    sm_update_driver_stats();
    
    sm_data->last_update_time = get_tick_count();
}

void sm_format_bytes(uint32_t bytes, char* buffer) {
    if (bytes < 1024) sprintf(buffer, "%d B", bytes);
    else if (bytes < 1024 * 1024) sprintf(buffer, "%d KB", bytes / 1024);
    else sprintf(buffer, "%d MB", bytes / (1024 * 1024));
}

void sm_format_time(uint32_t seconds, char* buffer) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    sprintf(buffer, "%02dh:%02dm:%02ds", hours, minutes, secs);
}

void sm_draw_bar_graph(int x, int y, int width, int height, uint32_t value, uint32_t max_value, uint8_t color) {
    vga_draw_rect(x, y, width, height, SM_BG_COLOR);
    vga_draw_rect(x, y, width, height, SM_BORDER_COLOR);
    if (max_value > 0 && value > 0) {
        int fill_width = (value * (width - 2)) / max_value;
        vga_draw_rect(x + 1, y + 1, fill_width, height - 2, color);
    }
}

void sm_draw_line_graph(int x, int y, int width, int height, uint32_t* data, int data_count, uint8_t color) {
    vga_draw_rect(x, y, width, height, SM_BG_COLOR);
    vga_draw_rect(x, y, width, height, SM_BORDER_COLOR);
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

void sm_draw_cpu_section(window_t* win, int x, int y, int width, int height) {
    vga_draw_text(x, y, "CPU", SM_HEADER_COLOR);
    sm_draw_bar_graph(x, y + 12, width - 10, 15, sm_data->cpu.usage_percent, 100, SM_CPU_COLOR);
    if (sm_data->show_graphs) {
        sm_draw_line_graph(x, y + 35, width - 10, 40, sm_data->cpu.history, SM_MAX_HISTORY, SM_CPU_COLOR);
    }
}

void sm_draw_ram_section(window_t* win, int x, int y, int width, int height) {
    vga_draw_text(x, y, "RAM", SM_HEADER_COLOR);
    sm_draw_bar_graph(x, y + 12, width - 10, 15, sm_data->ram.usage_percent, 100, SM_RAM_COLOR);
    if (sm_data->show_graphs) {
        sm_draw_line_graph(x, y + 35, width - 10, 40, sm_data->ram.history, SM_MAX_HISTORY, SM_RAM_COLOR);
    }
}

void sm_draw_window(window_t* win) {
    if (!sm_initialized || !sm_data) return;
    int px = win->x * 8;
    int py = win->y * 8;
    int width = win->w * 8;
    int height = win->h * 8;

    vga_draw_rect(px + 1, py + 1, width - 2, height - 2, SM_BG_COLOR);
    sm_draw_cpu_section(win, px + 5, py + 5, (width / 2) - 10, (height / 2) - 10);
    sm_draw_ram_section(win, px + (width / 2) + 5, py + 5, (width / 2) - 10, (height / 2) - 10);
}

void sm_handle_key(window_t* win, char c) {
    if (c == 'q' || c == 'Q') close_window(win->id);
}

void launch_system_monitor() {
    sm_init();
    // GÜNCELLEME: get_window_at veya get_window kontrolü
    int win_id = create_window("Sistem Monitoru", 5, 3, 80, 25, (CYAN << 4) | BLACK);
    window_t* win = get_window(win_id); 
    
    if (win) {
        win->draw_content = sm_draw_window;
        win->on_key = sm_handle_key;
        win->on_update = sm_update;
        win->data = sm_data;
    }
}