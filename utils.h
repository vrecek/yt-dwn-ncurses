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

#define BUF_SIZE 256


typedef enum {
    AUDIO_ONLY,
    AUDIO_VIDEO
} FileFormat;


typedef struct {
    char        output_path[BUF_SIZE];
    char        audio_ext[BUF_SIZE];
    FileFormat  type;
    int         success;
} Config;

typedef struct {
    char id[32];
    void (*fn)(WINDOW* win);
} Element;

typedef struct {
    char url[BUF_SIZE];
    char name[BUF_SIZE];
} VideoItem;



void print_greeting(WINDOW* win, int scr_width);
void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety, char* label);

void handle_mainmenu_input(WINDOW* win, char* buttons[], int btn_len, int* choice, int scr_width);

Config* init_config();



#endif