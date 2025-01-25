#include <string.h>
#include <ncurses.h>
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>


#define BUF_SIZE 256


char url[BUF_SIZE] = {0},
     msg[512] = {0};

int elements_len = 0,
    show_prompt  = 0,
    row15_color  = 4;

time_t dwn_curr  = 0,
       dwn_start = 0;

Element *elements = NULL;
Config  *config   = NULL;



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


int does_element_exist(char* id)
{
    for (size_t i = 0; i < elements_len; i++)
        if ( !strcmp(elements[i].id, id) )
            return 1;

    return 0;
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
    char fbuff[BUF_SIZE];
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
    wattron(win, COLOR_PAIR(row15_color));
    mvwprintw(win, 15, 5, msg);
    wattroff(win, COLOR_PAIR(row15_color));
}


void clear_row15(WINDOW* win)
{
    int w    = getmaxx(win),
        cond = w-2;

    char buffer[w];

    for (int i = 0; i < cond; i++)
        buffer[i] = ' ';

    buffer[cond] = '\0';

    mvwprintw(win, 15, 1, buffer);
}


void refresh_15row_output(WINDOW* win, void (*fn_trigger)(WINDOW* win))
{
    clear_row15(win);

    if (fn_trigger != NULL)
        fn_trigger(win);

    wrefresh(win);
}


void paste_colored(WINDOW* win, int color, char* message)
{
    wattron(win, COLOR_PAIR(color));
    mvwprintw(win, 15, 5, message);
    wattroff(win, COLOR_PAIR(color));
}


void print_colored(WINDOW* win, int color, char* message)
{
    clear_row15(win);

    paste_colored(win, color, message);

    wrefresh(win);
}


void url_prompt(WINDOW* win)
{
    mvwprintw(win, 15, 5, "Enter URL (q to cancel)");

    char urlbuff[BUF_SIZE],
         prntbuff[BUF_SIZE] = "> ";

    char* ptr = urlbuff;


    while (1)
    {
        mvwprintw(win, 16, 5, prntbuff);

        char ch = wgetch(win);

        if (ch != ERR)
        {
            if (ch == 'q')
            {
                remove_element("dwn_empty");

                if (url[0])
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

                if ( !strcmp(prntbuff, "> ") )
                {
                    url[0] = '\0';
                    remove_element("dwn_url");

                    break;
                }

                add_element("dwn_url", fn_show_url);
                strcpy(url, urlbuff);

                break;
            }

            *ptr++ = ch;
            *ptr   = '\0';

            sprintf(prntbuff, "> %s", urlbuff);
        }

        show_prompt = 0;
    }
}


void validate_ytdlp_command(char* buffer)
{
    if (config->format == AUDIO_ONLY)
        strcat(buffer, " -x");
}


int handle_dwnfromlink_input(WINDOW* win, char* buttons[], int btn_len, int* choice)
{
    if (show_prompt)
        url_prompt(win);

    switch (wgetch(win))
    {
        UpDownArrows;

        case '\n':
            row15_color = 4;
            msg[0]      = '\0';

            remove_element("dwn_output");
            remove_element("dwn_url");

            refresh_15row_output(win, NULL);

            if ( !strcmp(buttons[*choice], "Back") )
                return clear_return();

            else if ( !strcmp(buttons[*choice], "Enter url...") )
                show_prompt = 1;

            else if ( !strcmp(buttons[*choice], "Download") )
            {
                if (!url[0] || does_element_exist("dwn_empty"))
                {
                    add_element("dwn_empty", fn_show_empty_url);

                    time(&dwn_start);

                    return 0;
                }

                remove_element("dwn_url");

                strcpy(msg, "Starting download...");    
                refresh_15row_output(win, fn_show_dwn_output);
                
                sleep(1);

                char  buffer[512];
                FILE* pipe;
                int screen_width = getmaxx(win) - 8,
                    skip_buffer  = 0;

                sprintf(buffer, "yt-dlp -N 2 -P '%s' '%s'", config->download_path, url);
                validate_ytdlp_command(buffer);

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
                        refresh_15row_output(win, fn_show_dwn_output);

                        skip_buffer = 1;
                        continue;
                    }
                    else
                        strcpy(msg, buffer);

                    refresh_15row_output(win, fn_show_dwn_output);
                }

                pclose(pipe);

                url[0] = '\0';

                row15_color = 3;
                strcpy(msg, "Download completed");
                add_element("dwn_output", fn_show_dwn_output);
            }
    }

    return 0;
}


void handle_dnwfromfile(WINDOW* win, VideoItem* items, size_t items_len)
{
    int successes = 0;

    for (int i = 0; i < items_len; i++)
    {
        char buffer[BUF_SIZE];

        sprintf(buffer, "Downloading: %d/%d (%s)", successes+1, items_len, items[i].name);
        print_colored(win, 4, buffer);

        sleep(1);

        sprintf(buffer, "yt-dlp -N 2 -o '%s' -P '%s' '%s'", items[i].name, config->download_path, items[i].url);
        validate_ytdlp_command(buffer);

        FILE* pipe = popen(buffer, "r");

        if (!pipe)
        {
            sprintf(buffer, "[ERROR] Downloading #%d video failed.", successes+1);
            print_colored(win, 2, buffer);

            sleep(1);

            continue;
        }

        while ( fgets(buffer, sizeof(buffer), pipe) != NULL )
        {}

        successes++;

        fclose(pipe);
    }

    sprintf(msg, "Finished. Downloaded %d/%d videos", successes, items_len);
}


void download_from_link_handler(WINDOW* win, int scr_width)
{
    char *menu[]  = { "Enter url...", "Download", "Back" };

    int choice  = 0,
        btn_len = sizeof(menu) / sizeof(char*);

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
        print_buttons(win, menu, btn_len, &choice, 8, "[Download from link]");

        for (int i = 0; i < elements_len; i++)
            elements[i].fn(win);

        if (handle_dwnfromlink_input(win, menu, btn_len, &choice))
            return;

        wrefresh(win);
    }
}


void download_from_file_handler(WINDOW* win, int scr_width)
{
    char *menu[] = { "Download", "Back" };

    int choicev  = 0,
        btn_len  = sizeof(menu) / sizeof(char*),
        files_nr = 0,
        msg_clr  = 1;

    int       *choice = &choicev;
    FILE      *file   = fopen("videos.txt", "r");
    VideoItem *items  = NULL;


    if (file == NULL)
    {
        msg_clr = 2;
        sprintf(msg, "File 'videos.txt' could not be opened");
    }   
    else
    {
        char buffer[BUF_SIZE];

        while ( fgets(buffer, sizeof(buffer), file) )
        {
            char* token;
            int   name_len;

            files_nr++;

            items = (VideoItem*)realloc(items, sizeof(VideoItem) * files_nr);
            
            token = strtok(buffer, " ");
            strcpy( items[files_nr-1].url, token );

            token    = strtok(NULL, " ");
            name_len = strlen(token);

            if (token[name_len-1] == '\n')
                token[name_len-1] = '\0';

            strcpy( items[files_nr-1].name, token );
        }

        msg_clr = 3;
        sprintf(msg, "Found in 'videos.txt': %d", files_nr);

        fclose(file);
    }


    while (1)
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, scr_width);
        print_buttons(win, menu, btn_len, choice, 8, "[Download from file]");

        paste_colored(win, msg_clr, msg);
        
        switch (wgetch(win))
        {
            UpDownArrows;    

            case '\n':
                if ( !strcmp(menu[*choice], "Back") )
                {
                    free(items);
                    return; 
                }

                else if ( !strcmp(menu[*choice], "Download") )
                    handle_dnwfromfile(win, items, files_nr); 
        }

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

            else if ( !strcmp(buttons[*choice], "Download from file") )
                download_from_file_handler(win, scr_width);
    }
}


void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety, char* label)
{
    mvwprintw(win, 5, 5, label);

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
    const static char *GREETING  = "Welcome to ",
                      *GREET_CLR = "video downloader";

    static int center = 0,      
               init   = 0;

    int greet_len = strlen(GREETING);

    if (!init)
    {
        center = (scr_width - greet_len - strlen(GREET_CLR)) / 2;
        init   = 1;
    }

    mvwprintw(win, 1, center, GREETING);

    wattron(win, COLOR_PAIR(1));
    mvwprintw(win, 1, center+greet_len, GREET_CLR);
    wattroff(win, COLOR_PAIR(1));
}


Config* init_config()
{
    config = (Config*)malloc(sizeof(Config));

    strcpy(config->download_path, "/home/vrecek/Downloads");
    config->format = AUDIO_VIDEO;

    return config;
}