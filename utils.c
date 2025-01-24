#include <string.h>
#include <ncurses.h>
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>


#define UpDownArrows \
case 'w': \
    *choice = (*choice == 0) ? btn_len-1 : *choice-1; \
    break; \
case 's': \
    *choice = (*choice == btn_len-1) ? 0 : *choice+1; \
    break; \


typedef struct {
    char id[16];
    void (*fn)(WINDOW* win);
} Element;


const char *GREETING  = "Welcome to ",
           *GREET_CLR = "video downloader";

char url[128] = {0},
     msg[512] = {0};

int elements_len = 0,
    show_prompt  = 0;

time_t dwn_curr  = 0,
       dwn_start = 0;

Element* elements = NULL;



int print_greeting_get_center(int scr_width, int greet_len)
{
    static int center = 0;
    static int init   = 0;

    if (!init)
    {
        center = (scr_width - greet_len - strlen(GREET_CLR)) / 2;
        init   = 1;
    }

    return center;
}


void remove_element(char* id)
{
    for (int i = 0; i < elements_len; i++)
    {
        if ( !strcmp(elements[i].id, id) )
        {
            size_t new_size = 0;

            for (size_t j = 0; j < elements_len; j++)
            {
                if ( strcmp(elements[i].id, id) )
                    elements[new_size++] = elements[j];
            }

            elements     = (Element*)realloc(elements, sizeof(Element) * new_size);
            elements_len = new_size;

            return;
        }
    }
}


void add_element(char* id, void(*fn)(WINDOW* win))
{
    elements_len++;
    elements = (Element*)realloc(elements, sizeof(Element) * elements_len);

    strcpy(elements[elements_len-1].id, id);
    elements[elements_len-1].fn = fn;
}


int clear_return()
{
    dwn_curr = show_prompt = dwn_start = 0;

    free(elements);

    url[0] = '\0';

    elements     = NULL;
    elements_len = 0;

    return 1;
}


void fn_show_url(WINDOW* win)
{
    char fbuff[128];
    sprintf(fbuff, "URL: %s", url);
    
    wattron(win, COLOR_PAIR(3));
    mvwprintw(win, 15, 5, fbuff);
    wattroff(win, COLOR_PAIR(3));
}


void fn_show_empty_url(WINDOW* win)
{
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 15, 5, "URL not specified");
    wattroff(win, COLOR_PAIR(2));
}


void fn_show_dwn_output(WINDOW* win)
{
    wattron(win, COLOR_PAIR(4));
    mvwprintw(win, 15, 5, msg);
    wattroff(win, COLOR_PAIR(4));
}


void refresh_download_output(WINDOW* win, int show_msg_output)
{
    int  w    = getmaxx(win),
         cond = w-2;

    char buffer[w];

    for (int i = 0; i < cond; i++)
        buffer[i] = ' ';

    buffer[cond] = '\0';

    mvwprintw(win, 15, 1, buffer);

    if (show_msg_output)
        fn_show_dwn_output(win);

    wrefresh(win);
}


void url_prompt(WINDOW* win)
{
    mvwprintw(win, 15, 5, "Enter URL (q to cancel)");

    char* ptr = url;
    char  fbuff[128] = "> ";


    while (1)
    {
        mvwprintw(win, 16, 5, fbuff);

        char ch = wgetch(win);

        if (ch != ERR)
        {
            if (ch == 'q')
            {
                add_element("dwn_url", fn_show_url);
                break;
            }

            if (ch == '\n')
            {
                if (dwn_start)
                {
                    dwn_start = dwn_curr = 0;
                    remove_element("dwn_empty");
                }

                if ( !strcmp(fbuff, "> ") )
                {
                    url[0] = '\0';
                    remove_element("dwn_url");

                    break;
                }

                add_element("dwn_url", fn_show_url);
                break;
            }

            *ptr++ = ch;
            *ptr   = '\0';

            sprintf(fbuff, "> %s", url);
        }

        show_prompt = 0;
    }
}


int handle_dwnfromlink_input(WINDOW* win, char* buttons[], int btn_len, int* choice)
{
    if (show_prompt)
        url_prompt(win);

    switch (wgetch(win))
    {
        UpDownArrows;

        case '\n':
            msg[0] = '\0';
            remove_element("dwn_output");
            remove_element("dwn_url");

            refresh_download_output(win, 0);

            if ( !strcmp(buttons[*choice], "Back") )
                return clear_return();

            else if ( !strcmp(buttons[*choice], "Enter url...") )
                show_prompt = 1;

            else if ( !strcmp(buttons[*choice], "Download") )
            {
                if (!url[0])
                {
                    add_element("dwn_empty", fn_show_empty_url);

                    time(&dwn_start);

                    return 0;
                }

                remove_element("dwn_url");

                strcpy(msg, "Starting download...");    
                refresh_download_output(win, 1);
                
                sleep(1);

                char  buffer[512];
                FILE* pipe;
                int screen_width = getmaxx(win) - 8,
                    skip_buffer  = 0;

                sprintf(buffer, "yt-dlp '%s'", url);

                pipe = popen(buffer, "r");

                if (pipe == NULL)
                    return 0;

                while ( fgets(buffer, sizeof(buffer), pipe) != NULL )
                {
                    if (skip_buffer)
                        continue;

                    if (strlen(buffer) > screen_width)
                    {
                        strcpy(msg, "Downloading, please wait...");
                        refresh_download_output(win, 1);

                        skip_buffer = 1;
                        continue;
                    }
                    else
                        strcpy(msg, buffer);

                    refresh_download_output(win, 1);
                }

                pclose(pipe);

                url[0] = '\0';

                strcpy(msg, "Download completed");
                add_element("dwn_output", fn_show_dwn_output);
            }
    }

    return 0;
}

void download_from_link_handler(WINDOW* win, int scr_width)
{
    char *menu[]  = { "Enter url...", "Download", "Back" };

    int choice     = 0,
        btn_len    = sizeof(menu) / sizeof(char*),
        should_ret = 0;

    while (1)
    {
        werase(win);

        if (dwn_start)
        {
            time(&dwn_curr);

            if (difftime(dwn_curr, dwn_start) >= 2)
            {
                dwn_curr = dwn_start = 0;
                remove_element("dwn_empty");
            }
        }
        
        box(win, 0, 0);

        print_greeting(win, scr_width);
        print_buttons(win, menu, btn_len, &choice, 7);
        mvwprintw(win, 5, 5, "[Download from link]");

        for (int i = 0; i < elements_len; i++)
            elements[i].fn(win);

        if (handle_dwnfromlink_input(win, menu, btn_len, &choice))
            return;

        wrefresh(win);
    }
}

void handle_mainmenu_input(WINDOW* win, char* buttons[], int btn_len, int* choice, int scr_width)
{
    switch (wgetch(win))
    {
        UpDownArrows;

        case '\n':
            if ( !strcmp(buttons[*choice], "Quit") )
                *choice = 'q';

            else if ( !strcmp(buttons[*choice], "Download from link") )
                download_from_link_handler(win, scr_width);
    }
}

void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety)
{
    for (int i = 0; i < btn_len; i++)
    {
        if (i == *choice)
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, offsety+i, 5, buttons[i]);
            wattroff(win, A_REVERSE);
        }
        else
            mvwprintw(win, offsety+i, 5, buttons[i]);
    }
}

void print_greeting(WINDOW* win, int scr_width)
{
    int greet_len = strlen(GREETING),
        center    = print_greeting_get_center(scr_width, greet_len);

    mvwprintw(win, 1, center, GREETING);

    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, 1, center+greet_len, GREET_CLR);
    wattroff(win, COLOR_PAIR(1));
}

int* get_window_size(const WINDOW* win)
{
    int* arr = (int*)malloc(2 * sizeof(int));

    getmaxyx(win, arr[1], arr[0]);

    return arr;
}