#ifndef UTILS_PRIVATE_H
#define UTILS_PRIVATE_H


#include <ncurses.h>
#include "utils_typedefs.h"


void* start_animation(void* args);
void  finish_animation(pthread_t* animation_th);

void remove_element(char* id);
void add_element(char* id, void(*fn)(WINDOW* win));
int  does_element_exist(char* id);

void craft_ytdlp_command(char* buffer, char* url, char* name);
void open_path(char* path);
int  clear_return();

void get_metadata(Metadata* data, char* url, char* name);
void retrieve_full_filename(Metadata* metadata);
int  get_current_size(char* file);
int  check_download_error(char* output, char* error_container);

void fn_show_url(WINDOW* win);
void fn_show_name(WINDOW* win);
void fn_show_empty_url(WINDOW* win);
void fn_show_dwn_output(WINDOW* win);

void handle_dnwfromfile(WINDOW* win, VideoItem* items, size_t items_len);
void download_from_file_menu_set_videoobject(int* files_nr, int* msg_clr, VideoItem** items);
void craft_command_final(char* buffer, char* url, char* name, int* len);

void update_config_text_output(char* format, char* cookies, char* videopath);

void download_file_link(WINDOW* win);
int  handle_dwnfromlink(WINDOW* win, char* buttons[], int btn_len, int* choice);


#endif