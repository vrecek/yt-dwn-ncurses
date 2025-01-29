#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "utils_public.h"
#include "utils_private.h"



char f_url[BUF_SIZE]  = {0},
     f_name[BUF_SIZE] = {0},
     msg[BUF_SIZE]    = {0};

int elements_len     = 0,
    show_url_prompt  = 0,
    show_name_prompt = 0,
    row15_color      = 4;

time_t dwn_curr  = 0,
       dwn_start = 0;

Element *elements = NULL;
Config  *config   = NULL;



/***********************
    PRIVATE FUNCTIONS
***********************/

/*  ELEMENTS OBJECT HANDLERS  */

void remove_element(char* id)
{
    for (int i = 0; i < elements_len; i++)
    {
        if ( !strcmp(elements[i].id, id) )
        {
            size_t new_size = 0;

            for (size_t j = 0; j < elements_len; j++)
            {
                if ( strcmp(elements[j].id, id) )
                    elements[new_size++] = elements[j];
            }

            elements = (Element*)realloc(elements, sizeof(Element) * new_size);
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
    if (does_element_exist(id))
        return;

    elements_len++;
    elements = (Element*)realloc(elements, sizeof(Element) * elements_len);

    strcpy(elements[elements_len-1].id, id);
    elements[elements_len-1].fn = fn;
}



/*  COMMON PRIVATE FN  */

void validate_ytdlp_command(char* buffer)
{
    char local_buffer[BUF_SIZE];

    if (config->type == AUDIO_ONLY)
    {
        sprintf(local_buffer, " -x --audio-format %s", config->audio_ext);
        strcat(buffer, local_buffer);
    }

    if (config->cookies[0])
    {
        sprintf(local_buffer, " --cookies %s", config->cookies);
        strcat(buffer, local_buffer);
    }
}


int clear_return()
{
    dwn_curr = show_url_prompt = dwn_start = 0;

    free(elements);

    f_url[0]  = '\0';
    f_name[0] = '\0';

    elements     = NULL;
    elements_len = 0;

    return 1;
}



/*  DOWNLOAD OUTPUT INDICATORS  */

void fn_show_dwn_output(WINDOW* win)
{
    paste_colored(win, row15_color, 5, 15, msg);
}


void fn_show_url(WINDOW* win)
{
    char fbuff[BUF_SIZE];
    sprintf(fbuff, "URL: %s", f_url);
    
    paste_colored(win, 3, 5, 19, fbuff);
}


void fn_show_name(WINDOW* win)
{
    char fbuff[BUF_SIZE];
    sprintf(fbuff, "NAME: %s", f_name);
    
    paste_colored(win, 3, 5, 20, fbuff);
}


void fn_show_empty_url(WINDOW* win)
{
    paste_colored(win, 2, 5, 15, "URL not specified");
}



/*  DOWNLOAD FROM FILE FN  */

void handle_dnwfromfile(WINDOW* win, VideoItem* items, size_t items_len)
{
    int successes = 0;

    for (int i = 0; i < items_len; i++)
    {
        char buffer[BUF_SIZE];

        sprintf(buffer, "Downloading: %d/%d (%s)", successes+1, items_len, items[i].name);
        print_colored(win, 4, 5, 15, buffer);

        sprintf(buffer, "yt-dlp -N 2 -o '%s' -P '%s' '%s'", items[i].name, config->output_path, items[i].url);
        validate_ytdlp_command(buffer);

        FILE* pipe = popen(buffer, "r");

        if (!pipe)
        {
            sprintf(buffer, "[ERROR] Downloading #%d video failed.", successes+1);
            print_colored(win, 2, 5, 15, buffer);

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


void download_from_file_menu_set_videoobject(FILE* file, int* files_nr, int* msg_clr, VideoItem* items)
{
    char buffer[BUF_SIZE];

    while ( fgets(buffer, sizeof(buffer), file) )
    {
        char* token;
        int   name_len,
              files_sub = *files_nr-1;

        *files_nr++;

        items = (VideoItem*)realloc(items, sizeof(VideoItem) * *files_nr);
        
        token = strtok(buffer, " ");
        strcpy( items[files_sub].url, token );

        token    = strtok(NULL, " ");
        name_len = strlen(token);

        if (token[name_len-1] == '\n')
            token[name_len-1] = '\0';

        strcpy( items[files_sub].name, token );
    }

    if (! *files_nr)
        *msg_clr = 2;
    else
        *msg_clr = 3;

    sprintf(msg, "Videos in 'videos.txt': %d", *files_nr);

    fclose(file);
}



/*  DOWNLOAD FROM LINK FN  */

void download_file_link(WINDOW* win)
{
    if (!f_url[0] || does_element_exist("dwn_empty"))
    {
        nodelay(win, TRUE);

        add_element("dwn_empty", fn_show_empty_url);

        time(&dwn_start);

        return;
    }

    remove_element("dwn_name");
    remove_element("dwn_url");

    clear_row(win, 19);
    clear_row(win, 20);

    strcpy(msg, "Starting download...");    
    print_colored(win, 4, 5, 15, msg);
    
    char  buffer[512];
    FILE* pipe;
    int   screen_width = getmaxx(win) - 8,
          skip_buffer  = 0;

    sprintf(buffer, "yt-dlp -N 2 -P '%s' '%s'", config->output_path, f_url);
    validate_ytdlp_command(buffer);

    if (f_name[0])
        sprintf(buffer, "%s -o '%s'", buffer, f_name);

    pipe = popen(buffer, "r");

    if (pipe == NULL)
        return;

    while ( fgets(buffer, sizeof(buffer), pipe) != NULL )
    {
        if (skip_buffer)
            continue;

        if (strlen(buffer) > screen_width)
        {
            strcpy(msg, "Downloading, please wait...");
            print_colored(win, 4, 5, 15, msg);

            skip_buffer = 1;
            continue;
        }
        else
            strcpy(msg, buffer);

        print_colored(win, 4, 5, 15, msg);

    }

    pclose(pipe);

    f_url[0]    = '\0';
    f_name[0]   = '\0';
    row15_color = 3;

    strcpy(msg, "Download completed");
    add_element("dwn_output", fn_show_dwn_output);
}


int handle_dwnfromlink(WINDOW* win, char* buttons[], int btn_len, int* choice)
{
    char ch = wgetch(win);

    if (show_url_prompt)
    {
        display_prompt(win, f_url, "Enter URL (TAB to cancel)", &show_url_prompt, "dwn_url", fn_show_url);
        nodelay(win, FALSE);
    }
    else if (show_name_prompt)
    {
        display_prompt(win, f_name, "Enter filename (TAB to cancel)", &show_name_prompt, "dwn_name", fn_show_name);
        nodelay(win, FALSE);
    }


    switch (ch)
    {
        UpDownArrows;

        case '\n':
            row15_color = 4;
            msg[0]      = '\0';

            remove_element("dwn_output");

            clear_row(win, 15);

            if ( !strcmp(buttons[*choice], "Back") )
            {
                nodelay(win, FALSE);

                return clear_return();
            }

            else if ( !strcmp(buttons[*choice], "Enter url...") )
            {
                nodelay(win, TRUE);

                show_url_prompt = 1;
            }

            else if ( !strcmp(buttons[*choice], "Enter filename...") )
            {
                nodelay(win, TRUE);

                show_name_prompt = 1;
            }

            else if ( !strcmp(buttons[*choice], "Download") )
                download_file_link(win);
    }

    return 0;
}



/***********************
    PUBLIC FUNCTIONS
***********************/

/*  COMMON USAGE FN   */

void clear_row(WINDOW* win, int nr)
{
    int w    = getmaxx(win),
        cond = w-2;

    char buffer[w];

    for (int i = 0; i < cond; i++)
        buffer[i] = ' ';

    buffer[cond] = '\0';

    mvwprintw(win, nr, 1, buffer);
}


void paste_colored(WINDOW* win, int color, int x, int y, const char* message)
{
    wattron(win, COLOR_PAIR(color));
    mvwprintw(win, y, x, message);
    wattroff(win, COLOR_PAIR(color));
}


void print_colored(WINDOW* win, int color, int x, int y, const char* message)
{
    clear_row(win, y);

    paste_colored(win, color, x, y, message);

    wrefresh(win);
}


void print_buttons(WINDOW* win, char* buttons[], int btn_len, int* choice, int offsety, char* label)
{
    int clr = 0;

    mvwprintw(win, 5, 5, label);

    for (int i = 0; i < btn_len; i++)
    {
        if ( !strcmp(buttons[i], "Back") )
            clr = 4;
        else if ( !strcmp(buttons[i], "Quit") )
            clr = 2;

        wattron(win, COLOR_PAIR(clr));

        if (i == *choice)
        {
            wattron(win, A_REVERSE);
            mvwprintw(win, offsety+i, 5, buttons[i]);
            wattroff(win, A_REVERSE);
        }
        else
            mvwprintw(win, offsety+i, 5, buttons[i]);

        wattroff(win, COLOR_PAIR(clr));
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

    mvwprintw(win, 2, center, GREETING);
    paste_colored(win, 1, center+greet_len, 2, GREET_CLR);
}


void display_prompt(WINDOW* win, char* varbuffer, char* prompt_info, int* show_prompt, char* element_id, void (*fn)(WINDOW* win))
{
    mvwprintw(win, 15, 5, prompt_info);

    char msgbuff[BUF_SIZE]  = {0},
         prntbuff[BUF_SIZE] = "> ";

    char* ptr = msgbuff;

    *show_prompt = 0;

    while (1)
    {
        mvwprintw(win, 16, 5, prntbuff);

        char ch = wgetch(win);

        if (ch != ERR)
        {
            if (ch == '\t')
                break;

            if (ch == '\n')
            {
                if ( !strcmp(prntbuff, "> ") )
                {
                    varbuffer[0] = '\0';
                    remove_element(element_id);

                    break;
                }

                strcpy(varbuffer, msgbuff);

                break;
            }

            if (ch == 127)
            {
                if (!msgbuff[0])
                    continue;

                *(--ptr) = '\0';
            }
            else
            {
                *ptr++ = ch;
                *ptr   = '\0';
            }

            clear_row(win, 16);
            sprintf(prntbuff, "> %s", msgbuff);
        }
    }

    dwn_start = dwn_curr = 0;

    remove_element("dwn_empty");

    if (varbuffer[0] && !does_element_exist(element_id))
        add_element(element_id, fn);
}



/* MAIN MENU HANDLERS */

void download_from_link_menu(WINDOW* win, int scr_width)
{
    char *menu[]  = { "Enter url...", "Enter filename...", "Download", "Back" };

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
                nodelay(win, FALSE);

                dwn_curr = dwn_start = 0;
                remove_element("dwn_empty");
            }
        }
        
        box(win, 0, 0);

        print_greeting(win, scr_width);
        print_buttons(win, menu, btn_len, &choice, 8, "[Download from link]");

        for (int i = 0; i < elements_len; i++)
            elements[i].fn(win);

        if (handle_dwnfromlink(win, menu, btn_len, &choice))
            return;

        wrefresh(win);
    }
}


void download_from_file_menu(WINDOW* win, int scr_width)
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
        download_from_file_menu_set_videoobject(file, &files_nr, &msg_clr, items);


    while (1)
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, scr_width);
        print_buttons(win, menu, btn_len, choice, 8, "[Download from file]");

        paste_colored(win, msg_clr, 5, 15, msg);
        
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
                {
                    if (!files_nr)
                        break;

                    handle_dnwfromfile(win, items, files_nr); 
                }
        }

        wrefresh(win);
    }
}


void read_config_menu(WINDOW* win, int scr_width)
{
    int   choice = 0;
    char *menu[] = { "Back" };
    char  buffer[BUF_SIZE],
          cookies[BUF_SIZE],
          format[32];


    if (config->type == AUDIO_ONLY)
        strcpy(format, "Audio only");
    else if (config->type == AUDIO_VIDEO)
        strcpy(format, "Audio+Video");

    if (!config->cookies[0] || config->cookies[0] == '0')
        strcpy(cookies, "None");
    else
        strcpy(cookies, config->cookies);


    while (1)
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, scr_width);
        print_buttons(win, menu, 1, &choice, 20, "[Configuration details]");

        paste_colored(win, 4, 5, 8, "config.conf");


        if (!config->success)
        {
            wattron(win, COLOR_PAIR(2));

            mvwprintw(win, 16, 5, "[ERROR] Failed reading 'config.conf'. Default values will be used");
            mvwprintw(win, 17, 5, "[ERROR] Check your configuration file");

            wattroff(win, COLOR_PAIR(2));
        }
        else
        {
            sprintf(buffer, "[Output path]:    %s", config->output_path);
            mvwprintw(win, 10, 5, buffer);

            sprintf(buffer, "[Format type]:    %s", format);
            mvwprintw(win, 11, 5, buffer);

            sprintf(buffer, "[Audio-only ext]: %s", config->audio_ext);
            mvwprintw(win, 12, 5, buffer);

            sprintf(buffer, "[Cookies]:        %s", cookies);
            mvwprintw(win, 13, 5, buffer);
        }


        if (wgetch(win) == '\n')
            return;


        wrefresh(win);
    }
}



/*  CONFIG FN  */

Config* init_config()
{
    config = (Config*)malloc(sizeof(Config));

    int   len;
    char* token;
    char  buffer[BUF_SIZE],
          key[BUF_SIZE];

    FILE* file = fopen("config.conf", "r");


    if (file == NULL)
    {
        strcpy(config->output_path, "~/Downloads");
        strcpy(config->audio_ext, "mp3");

        config->cookies[0] = '\0';
        config->type       = AUDIO_VIDEO;
        config->success    = 0;

        return config;
    }

    while ( fgets(buffer, sizeof(buffer), file) != NULL )
    {
        if (buffer[0] == '#' || buffer[0] == '\n')
            continue;

        token = strtok(buffer, " ");

        strcpy(key, token);

        token = strtok(NULL, " ");
        len   = strlen(token);

        if (token[len-1] == '\n')
            token[len-1] = '\0';


        if ( !strcmp(key, "output_path") )
            strcpy(config->output_path, token);

        else if ( !strcmp(key, "audio_ext") )
            strcpy(config->audio_ext, token);

        else if ( !strcmp(key, "cookies") )
            if (token[0] == '0')
                config->cookies[0] = '\0';
            else
                strcpy(config->cookies, token);

        else if ( !strcmp(key, "type") )
            config->type = token[0] - '0';
    }

    fclose(file);

    config->success = 1;

    return config;
}