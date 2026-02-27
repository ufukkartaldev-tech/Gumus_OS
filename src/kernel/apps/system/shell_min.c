#include "shell.h"
#include "string.h"
#include "kernel.h"
#include "fs.h"
#include "sound.h"

static char buf[MAX_COMMAND_LEN];
static int idx = 0;

void shell_init() {
    idx = 0;
    memset(buf, 0, MAX_COMMAND_LEN);
    print_color("\n> ", 11);
}

static void trim(char* s) {
    int i = 0;
    while (s[i] == ' ') i++;
    int j = 0;
    while (s[i]) s[j++] = s[i++];
    s[j] = 0;
    int k = j - 1;
    while (k >= 0 && s[k] == ' ') s[k--] = 0;
}

void shell_parse_command(char* cmd) {
    trim(cmd);
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "yardim") == 0) {
        print_color("\nKomutlar:\n", 14);
        print("  help         \n");
        print("  clear        \n");
        print("  versiyon     \n");
        print("  ls           \n");
        print("  oku <ad>     \n");
        print("  kaydet <ad> <icerik>\n");
        print("  beep [f ms]  \n");
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "temizle") == 0) {
        clear_screen();
    } else if (strcmp(cmd, "versiyon") == 0) {
        print_color("\nGumusOS minimal shell\n", 13);
    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "listele") == 0) {
        fs_ls();
    } else if (strncmp(cmd, "oku ", 4) == 0) {
        char* name = cmd + 4;
        trim(name);
        if (strlen(name) > 0) fs_cat(name);
    } else if (strncmp(cmd, "beep", 4) == 0) {
        uint32_t f = 1000, ms = 150;
        // parse optional args: beep <freq> <ms>
        if (strlen(cmd) > 4) {
            char* p = cmd + 4;
            while (*p == ' ') p++;
            if (*p) {
                f = atoi(p);
                while (*p && *p != ' ') p++;
                while (*p == ' ') p++;
                if (*p) ms = atoi(p);
            }
        }
        // crude delay
        play_sound(f);
        volatile uint32_t c = ms * 5000;
        while (c--) { __asm__ volatile("" ::: "memory"); }
        nosound();
    } else if (strncmp(cmd, "kaydet ", 7) == 0) {
        char* rest = cmd + 7;
        trim(rest);
        int p = 0;
        while (rest[p] && rest[p] != ' ') p++;
        if (p > 0) {
            rest[p] = 0;
            char* content = rest + p + 1;
            fs_write(rest, content);
            print_color("\nOK\n", 10);
        }
    } else if (strlen(cmd) > 0) {
        print_color("\nBilinmeyen komut\n", 12);
    }
}

void shell_input(char c) {
    if (c == '\r' || c == '\n') {
        buf[idx] = 0;
        shell_parse_command(buf);
        idx = 0;
        memset(buf, 0, MAX_COMMAND_LEN);
        print_color("\n> ", 11);
        return;
    }
    if (c == '\b') {
        if (idx > 0) idx--;
        return;
    }
    if (idx < MAX_COMMAND_LEN - 1) {
        buf[idx++] = c;
    }
}
