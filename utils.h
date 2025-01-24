#ifndef UTILS_H
#define UTILS_H


#include <ncurses.h>


void print_greeting(WINDOW* win, int scr_width);
void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety);

void handle_mainmenu_input(WINDOW* win, char* buttons[], int btn_len, int* choice, int scr_width);

int* get_window_size(const WINDOW* win);


#endif