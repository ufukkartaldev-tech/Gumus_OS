#include "keyboard.h"
#include "io.h"
#include "kernel.h"
#include "shell.h"
#include "window.h"

// Forward declaration
extern void kbd_put(char c);

void handle_keyboard(uint8_t scancode) {
    if (scancode & 0x80) {
        // Key released
        return;
    }
    
    char c = keyboard_map[scancode];
    if (c != 0) {
        kbd_put(c);
        if (!on_window_key_event(c)) {
            shell_input(c);
        }
    }
}
