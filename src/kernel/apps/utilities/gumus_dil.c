#include "gumus_dil.h"
#include "kernel.h"
#include "memory.h"
#include "string.h"
#include "vga_gfx.h"

// GÃ¼mÃ¼ÅŸDil (v0.2.0) - Modern TÃ¼rkÃ§e SÃ¶zdizimi
void gumus_execute(const char* input) {
    char cmd[256];
    strncpy(cmd, input, 255);
    
    // Basit Tokenizer (Sadece ilk kelime komut)
    char* token = strtok(cmd, " ()\",");
    if (!token) return;

    if (strcmp(token, "yaz") == 0) {
        char* text = strtok(NULL, ")\"");
        if (text) {
            print("\n[GumusDil] ");
            print(text);
        }
    } 
    else if (strcmp(token, "nokta") == 0) {
        int x = atoi(strtok(NULL, ", "));
        int y = atoi(strtok(NULL, ", "));
        int r = atoi(strtok(NULL, ", "));
        vga_putpixel(x, y, (uint8_t)r);
    }
    else if (strcmp(token, "kutu") == 0) {
        int x = atoi(strtok(NULL, ", "));
        int y = atoi(strtok(NULL, ", "));
        int w = atoi(strtok(NULL, ", "));
        int h = atoi(strtok(NULL, ", "));
        int r = atoi(strtok(NULL, ", "));
        vga_draw_rect(x, y, w, h, (uint8_t)r);
    }
    else if (strcmp(token, "temizle") == 0) {
        int color = atoi(strtok(NULL, ", "));
        vga_clear((uint8_t)color);
    }
    else if (strcmp(token, "goster") == 0) {
        vga_present();
    }
    else if (strcmp(token, "bekle") == 0) {
        int ms = atoi(strtok(NULL, ", "));
        msleep(ms);
    }
    else if (strcmp(token, "ses") == 0) {
        int hz = atoi(strtok(NULL, ", "));
        int ms = atoi(strtok(NULL, ", "));
        beep_hz(hz, ms);
    }
    else if (strcmp(token, "dongu") == 0) {
        // Ä°leride eklenecek
        print("\n[GumusDil] Dongu henuz desteklenmiyor.");
    }
    else {
        print("\n[GumusDil Hata] Ne dedin? '");
        print(token);
        print("' anlasilmadi.");
    }
}
