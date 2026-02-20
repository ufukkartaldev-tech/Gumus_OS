#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "window.h"

void init_file_manager();
void show_file_manager_window();
void file_manager_draw(window_t* win);
void file_manager_click(window_t* win, int x, int y);

#endif
