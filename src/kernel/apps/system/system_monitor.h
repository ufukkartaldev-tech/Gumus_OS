#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H

#include <stdint.h>
#include "window.h"
#include "vga_gfx.h"

// Sistem MonitÃ¶rÃ¼ Sabitleri
#define SM_MAX_HISTORY 60           // 60 saniye geÃ§miÅŸ
#define SM_BAR_HEIGHT 20
#define SM_GRAPH_HEIGHT 100
#define SM_SECTION_SPACING 10
#define SM_TEXT_HEIGHT 12

// Sistem MonitÃ¶rÃ¼ Renkleri
#define SM_BG_COLOR 15             // Beyaz
#define SM_BORDER_COLOR 8          // Gri
#define SM_TEXT_COLOR 0            // Siyah
#define SM_CPU_COLOR 4             // KÄ±rmÄ±zÄ±
#define SM_RAM_COLOR 2             // YeÅŸil
#define SM_GRID_COLOR 7            // AÃ§Ä±k gri
#define SM_HEADER_COLOR 1          // Mavi

// CPU Ä°statistikleri
typedef struct {
    uint32_t total_ticks;
    uint32_t idle_ticks;
    uint32_t user_ticks;
    uint32_t kernel_ticks;
    uint32_t usage_percent;
    uint32_t history[SM_MAX_HISTORY];
    int history_index;
} cpu_stats_t;

// RAM Ä°statistikleri
typedef struct {
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t free_memory;
    uint32_t kernel_memory;
    uint32_t user_memory;
    uint32_t usage_percent;
    uint32_t history[SM_MAX_HISTORY];
    int history_index;
} ram_stats_t;

// Sistem Bilgileri
typedef struct {
    uint32_t uptime_seconds;
    uint32_t total_processes;
    uint32_t active_processes;
    uint32_t total_interrupts;
    uint32_t context_switches;
} system_info_t;

// SÃ¼rÃ¼cÃ¼ Ä°statistikleri
typedef struct {
    uint32_t disk_reads;
    uint32_t disk_writes;
    uint32_t network_packets_sent;
    uint32_t network_packets_received;
    uint32_t keyboard_presses;
    uint32_t mouse_clicks;
} driver_stats_t;

// Sistem MonitÃ¶rÃ¼ Pencere Verisi
typedef struct {
    cpu_stats_t cpu;
    ram_stats_t ram;
    system_info_t system;
    driver_stats_t drivers;
    
    int update_counter;
    int refresh_rate; // Her kaÃ§ tick'te bir gÃ¼ncellenecek
    uint32_t last_update_time;
    
    int show_graphs;
    int show_details;
} sm_data_t;

// Sistem MonitÃ¶rÃ¼ FonksiyonlarÄ±
void sm_init();
void sm_update(window_t* win);
void sm_draw_window(window_t* win);
void sm_handle_click(window_t* win, int x, int y);
void sm_handle_key(window_t* win, char c);

// Ä°statistik toplama fonksiyonlarÄ±
void sm_update_cpu_stats();
void sm_update_ram_stats();
void sm_update_system_info();
void sm_update_driver_stats();

// Ã‡izim fonksiyonlarÄ±
void sm_draw_cpu_section(window_t* win, int x, int y, int width, int height);
void sm_draw_ram_section(window_t* win, int x, int y, int width, int height);
void sm_draw_system_info_section(window_t* win, int x, int y, int width, int height);
void sm_draw_driver_stats_section(window_t* win, int x, int y, int width, int height);

// Grafik Ã§izim fonksiyonlarÄ±
void sm_draw_bar_graph(int x, int y, int width, int height, uint32_t value, uint32_t max_value, uint8_t color);
void sm_draw_line_graph(int x, int y, int width, int height, uint32_t* data, int data_count, uint8_t color);
void sm_draw_grid(int x, int y, int width, int height, int grid_size);

// Utility fonksiyonlarÄ±
void sm_format_bytes(uint32_t bytes, char* buffer);
void sm_format_time(uint32_t seconds, char* buffer);
uint32_t sm_get_uptime();
uint32_t sm_get_cpu_usage();
uint32_t sm_get_ram_usage();

// Ana giriÅŸ fonksiyonu
void launch_system_monitor();

#endif
