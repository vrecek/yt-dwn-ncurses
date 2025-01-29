#ifndef UTILS_PUBLIC_H
#define UTILS_PUBLIC_H


#include <ncurses.h>
#include "utils_typedefs.h"


void clear_row(WINDOW* win, int nr);

void paste_colored(WINDOW* win, int color, int x, int y, const char* message);
void print_colored(WINDOW* win, int color, int x, int y, const char* message);

void print_greeting(WINDOW* win, int scr_width);
void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety, char* label);
void display_prompt(WINDOW* win, char* varbuffer, char* prompt_info, int* show_prompt, char* element_id, void (*fn)(WINDOW* win));

void download_from_file_menu(WINDOW* win, int scr_width);
void read_config_menu(WINDOW* win, int scr_width);
void download_from_link_menu(WINDOW* win, int scr_width);

Config* init_config();


#endif