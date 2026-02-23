#include "snake.h"
#include "window.h"
#include "kernel.h"
#include "memory.h"
#include "vga_gfx.h"
#include "memory.h"
#include "vga_gfx.h"
#include "string.h"
#include "mouse.h"

// Forward Declaration needed for set_window_update_callback
void snake_update(window_t* win);

#define SNAKE_MAX_LEN 100
#define GAME_WIDTH 30
#define GAME_HEIGHT 20
#define CELL_SIZE 5

// YÄ±lan YÃ¶nleri (WASD veya Ok TuÅŸlarÄ±)
// dx, dy: -1, 0, 1

typedef struct {
    int x[SNAKE_MAX_LEN];
    int y[SNAKE_MAX_LEN];
    int length;
    int dx, dy;
    
    int food_x, food_y;
    int score;
    int game_over;
    
    uint32_t last_update;
} snake_game_t;

// Basit Rastgele SayÄ± Ãœreteci (Linear Congruential Generator)
static uint32_t rand_seed = 123456789;
uint32_t rand() {
    rand_seed = (rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return rand_seed;
}

void spawn_food(snake_game_t* game) {
    game->food_x = rand() % GAME_WIDTH;
    game->food_y = rand() % GAME_HEIGHT;
}

// snake_update forward declaration is above.

void snake_update(window_t* win) {
    snake_game_t* game = (snake_game_t*)win->data;
    if (!game) return;

    // Oyun zamanlamasÄ± (Update Logic)
    uint32_t now = get_timer_ticks();
    if (!game->game_over && (now - game->last_update) > 2) { // HÄ±z kontrolÃ¼
        game->last_update = now;
        
        // Hareket vs... (Eski logic buraya)
        // GÃ¶vdeyi kaydÄ±r
        for (int i = game->length - 1; i > 0; i--) {
            game->x[i] = game->x[i-1];
            game->y[i] = game->y[i-1];
        }
        // KafayÄ± ilerlet
        game->x[0] += game->dx;
        game->y[0] += game->dy;
        
        // Game Over Kontrolleri
        if (game->x[0] < 0 || game->x[0] >= GAME_WIDTH || 
            game->y[0] < 0 || game->y[0] >= GAME_HEIGHT) {
            game->game_over = 1;
        }
        for (int i = 1; i < game->length; i++) {
            if (game->x[0] == game->x[i] && game->y[0] == game->y[i]) {
                game->game_over = 1;
            }
        }
        
        // Yemek
        if (game->x[0] == game->food_x && game->y[0] == game->food_y) {
            game->score += 10;
            if (game->length < SNAKE_MAX_LEN) game->length++;
            spawn_food(game);
        }
        
        // EkranÄ± Yenile
        // Sadece bu pencere iÃ§in deÄŸil, tÃ¼m sahne iÃ§in (Z-Order bozulmasÄ±n diye)
        // TODO: Sadece pencereyi Ã§izip altÄ±na/Ã¼stÃ¼ne gelenleri clipleyerek performans artÄ±rÄ±labilir.
        draw_windows();
        draw_mouse_cursor(); // Ä°mleci en Ã¼ste koy
    }
}

void snake_draw(window_t* win) {
    snake_game_t* game = (snake_game_t*)win->data;
    if (!game) return;
    
    int px = win->x * 8;
    int py = win->y * 8;
    int content_x = px + 4;
    int content_y = py + 12;
    // Logic removed, only drawing now
    
    // Ã‡izim
    
    // Arkaplan
    vga_draw_rect(content_x, content_y, GAME_WIDTH * CELL_SIZE, GAME_HEIGHT * CELL_SIZE, 0); // Siyah
    
    // YÄ±lan
    for (int i = 0; i < game->length; i++) {
        uint8_t color = (i == 0) ? 10 : 2; // Kafa aÃ§Ä±k yeÅŸil, gÃ¶vde koyu yeÅŸil
        vga_draw_rect(content_x + game->x[i] * CELL_SIZE, 
                      content_y + game->y[i] * CELL_SIZE, 
                      CELL_SIZE - 1, CELL_SIZE - 1, color);
    }
    
    // Yemek
    vga_draw_rect(content_x + game->food_x * CELL_SIZE, 
                  content_y + game->food_y * CELL_SIZE, 
                  CELL_SIZE - 1, CELL_SIZE - 1, 4); // KÄ±rmÄ±zÄ±
                  
    // Puan
    char score_buf[16];
    itoa(game->score, score_buf);
    vga_draw_text(content_x, content_y - 10, "Puan:", 0);
    vga_draw_text(content_x + 40, content_y - 10, score_buf, 0);
    
    if (game->game_over) {
        vga_draw_text(content_x + 20, content_y + 40, "GAME OVER", 4);
        vga_draw_text(content_x + 10, content_y + 50, "Yeniden icin 'R'", 15);
    }
}

void snake_key(window_t* win, char c) {
    snake_game_t* game = (snake_game_t*)win->data;
    if (!game) return;
    
    if (game->game_over) {
        if (c == 'r' || c == 'R') {
            // Reset
            game->length = 3;
            game->x[0] = 5; game->y[0] = 5;
            game->x[1] = 4; game->y[1] = 5;
            game->x[2] = 3; game->y[2] = 5;
            game->dx = 1; game->dy = 0;
            game->score = 0;
            game->game_over = 0;
            spawn_food(game);
        }
        return;
    }
    
    if (c == 'w' || c == 'W') { if (game->dy == 0) { game->dx = 0; game->dy = -1; } }
    if (c == 's' || c == 'S') { if (game->dy == 0) { game->dx = 0; game->dy = 1; } }
    if (c == 'a' || c == 'A') { if (game->dx == 0) { game->dx = -1; game->dy = 0; } }
    if (c == 'd' || c == 'D') { if (game->dx == 0) { game->dx = 1; game->dy = 0; } }
}

void snake_close(window_t* win) {
    if (win->data) {
        kfree(win->data);
    }
}

void init_snake_game() {
    snake_game_t* game = (snake_game_t*)kmalloc(sizeof(snake_game_t));
    
    // VarsayÄ±lanlar
    game->length = 3;
    game->x[0] = 5; game->y[0] = 5;
    game->x[1] = 4; game->y[1] = 5;
    game->x[2] = 3; game->y[2] = 5;
    game->dx = 1; game->dy = 0;
    game->score = 0;
    game->game_over = 0;
    game->last_update = get_timer_ticks();
    
    rand_seed = get_timer_ticks(); // Basit seed
    spawn_food(game);
    
    int win_id = create_window("YILAN OYUNU", 5, 2, 22, 18, (1 << 4) | 15); // Mavi
    
    set_window_callbacks(win_id, snake_draw, 0);
    set_window_key_callback(win_id, snake_key);
    set_window_update_callback(win_id, snake_update);
    set_window_data(win_id, game, snake_close);
}
