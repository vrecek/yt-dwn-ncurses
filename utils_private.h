#ifndef UTILS_PRIVATE_H
#define UTILS_PRIVATE_H


#include <ncurses.h>
#include "utils_typedefs.h"


void remove_element(char* id);
void add_element(char* id, void(*fn)(WINDOW* win));
int  does_element_exist(char* id);

void validate_ytdlp_command(char* buffer);
int  clear_return();

void fn_show_url(WINDOW* win);
void fn_show_name(WINDOW* win);
void fn_show_empty_url(WINDOW* win);
void fn_show_dwn_output(WINDOW* win);

void handle_dnwfromfile(WINDOW* win, VideoItem* items, size_t items_len);
void download_from_file_menu_set_videoobject(FILE* file, int* files_nr, int* msg_clr, VideoItem* items);

void download_file_link(WINDOW* win);
int  handle_dwnfromlink(WINDOW* win, char* buttons[], int btn_len, int* choice);


#endif