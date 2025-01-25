#ifndef UTILS_H
#define UTILS_H


#include <ncurses.h>


#define UpDownArrows \
case 'w': \
    *choice = (*choice == 0) ? btn_len-1 : *choice-1; \
    break; \
case 's': \
    *choice = (*choice == btn_len-1) ? 0 : *choice+1; \
    break; \


typedef enum {
    AUDIO_ONLY,
    AUDIO_VIDEO
} FileFormat;


typedef struct {
    char download_path[128];
    FileFormat format;
} Config;

typedef struct {
    char id[16];
    void (*fn)(WINDOW* win);
} Element;

typedef struct {
    char url[128];
    char name[64];
} VideoItem;



void print_greeting(WINDOW* win, int scr_width);
void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety, char* label);

void handle_mainmenu_input(WINDOW* win, char* buttons[], int btn_len, int* choice, int scr_width);

Config* init_config();



#endif