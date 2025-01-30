#ifndef UTILS_TYPEDEFS_H
#define UTILS_TYPEDEFS_H



#include <ncurses.h>



#define BUF_SIZE 256


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
    WINDOW* win;
    int x;
    int y;
} AnimationArgs;


typedef struct {
    char        output_path[BUF_SIZE];
    char        audio_ext[BUF_SIZE];
    char        cookies[BUF_SIZE];
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



#endif