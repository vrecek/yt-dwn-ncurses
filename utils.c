#include <string.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "utils_public.h"
#include "utils_private.h"



char f_url[BUF_SIZE]  = {0},
     f_name[BUF_SIZE] = {0},
     msg[BUF_SIZE]    = {0};

int elements_len     = 0,
    show_url_prompt  = 0,
    show_name_prompt = 0,
    row15_color      = 4;

time_t timer_curr  = 0,
       timer_start = 0;

Element *elements = NULL;
Config  *config   = NULL;

pthread_mutex_t th_lock;



/***********************
    PRIVATE FUNCTIONS
***********************/

/*  ANIMATIONS  */

void* start_animation(void* args)
{
    AnimationArgs* obj = (AnimationArgs*)args;

    char* frames[] = { "[   ]", "[-  ]", "[-- ]", "[---]" };

    int frames_length = sizeof(frames) / sizeof(char*),
        current_frame = 0;

    while (1)
    {
        sleep(1);

        if (current_frame >= frames_length)
            current_frame = 0;

        pthread_mutex_lock(&th_lock);
        print_colored(obj->win, 4, obj->x, obj->y, frames[current_frame++]);
        pthread_mutex_unlock(&th_lock);
    }

    return NULL;
}


void finish_animation(pthread_t* animation_th)
{
    pthread_cancel(*animation_th);
    pthread_join(*animation_th, NULL);
    pthread_mutex_destroy(&th_lock);
}


void init_animation(pthread_t* animation_th, AnimationArgs* args)
{
    pthread_mutex_init(&th_lock, NULL);
    pthread_create(animation_th, NULL, start_animation, (void*)args);
}



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

    strlcpy(elements[elements_len-1].id, id, 32);
    elements[elements_len-1].fn = fn;
}



/*  COMMON PRIVATE FN  */

void craft_ytdlp_command(char* buffer, char* url, char* name)
{
    int len;

    buffer[0] = '\0';

    snprintf(buffer, BUF_SIZE, "yt-dlp -N 2 -P '%s'", config->output_path);
    len = strlen(buffer);
    
    if (config->type == AUDIO_ONLY)
    {
        snprintf(buffer+len, BUF_SIZE-len, " -x --audio-format %s", config->audio_ext);
        len = strlen(buffer);
    }

    if (config->cookies[0])
    {
        snprintf(buffer+len, BUF_SIZE-len, " --cookies '%s'", config->cookies);
        len = strlen(buffer);
    }

    if (name[0])
    {
        snprintf(buffer+len, BUF_SIZE-len, " -o '%s'", name);
        len = strlen(buffer);
    }

    snprintf(buffer+len, BUF_SIZE-len, " -- '%s' 2>&1", url);
}


int clear_return()
{
    timer_curr = show_url_prompt = timer_start = 0;

    free(elements);

    f_url[0]  = '\0';
    f_name[0] = '\0';

    elements     = NULL;
    elements_len = 0;

    return 1;
}


void open_path(char* path)
{
    char buffer[BUF_SIZE];

    snprintf(buffer, BUF_SIZE, "xdg-open %s", path);
    system(buffer);
}


void update_config_text_output(char* format, char* cookies)
{
    if (config->type == AUDIO_ONLY)
        strcpy(format, "Audio only");
    else if (config->type == AUDIO_VIDEO)
        strcpy(format, "Audio+Video");


    if (!config->cookies[0] || config->cookies[0] == '0')
        strcpy(cookies, "None");
    else
        strlcpy(cookies, config->cookies, BUF_SIZE);
}


int check_download_error(char* output, char* error_container)
{
    if (strstr(output, "valid URL"))
        strcpy(error_container, "[ERROR] URL is incorrect");

    else if (strstr(output, "Video unavailable"))
        strcpy(error_container, "[ERROR] Video is unavailable");

    return error_container[0];
}



/*  DOWNLOAD OUTPUT INDICATORS  */

void fn_show_dwn_output(WINDOW* win)
{
    paste_colored(win, row15_color, 5, 15, msg);
}


void fn_show_url(WINDOW* win)
{
    char fbuff[BUF_SIZE];
    snprintf(fbuff, BUF_SIZE, "URL: %s", f_url);
    
    paste_colored(win, 3, 5, 19, fbuff);
}


void fn_show_name(WINDOW* win)
{
    char fbuff[BUF_SIZE];
    snprintf(fbuff, BUF_SIZE, "NAME: %s", f_name);
    
    paste_colored(win, 3, 5, 20, fbuff);
}


void fn_show_empty_url(WINDOW* win)
{
    paste_colored(win, 2, 5, 15, "URL not specified");
}



/*  DOWNLOAD FROM FILE FN  */

void handle_dnwfromfile(WINDOW* win, VideoItem* items, size_t items_len)
{
    int successes    = 0,
        skip_buffer  = 0,
        screen_width = getmaxx(win) - 10;

    char buffer[BUF_SIZE],
         error[BUF_SIZE];

    AnimationArgs args = {win, 5, 16};
    pthread_t     animation_th;

    FILE* pipe = NULL;


    init_animation(&animation_th, &args);

    for (int i = 0; i < items_len; i++)
    {
        error[0] = '\0';

        pthread_mutex_lock(&th_lock);
        print_colored(win, 4, 5, 15, "Fetching URL...");
        pthread_mutex_unlock(&th_lock);

        craft_ytdlp_command(buffer, items[i].url, items[i].name);

        pipe = popen(buffer, "r");

        if (!pipe)
            continue;


        while ( fgets(buffer, sizeof(buffer), pipe) != NULL )
        {
            if (check_download_error(buffer, error))
            {
                pthread_mutex_lock(&th_lock);

                print_colored(win, 2, 5, 15, error);
                sleep(2);

                pthread_mutex_unlock(&th_lock);

                break;
            }

            if (skip_buffer) continue;

            snprintf(buffer, BUF_SIZE, "Downloading: %d/%d (%s)", i+1, items_len, items[i].name);

            pthread_mutex_lock(&th_lock);
            print_colored(win, 4, 5, 15, buffer);
            pthread_mutex_unlock(&th_lock);

            skip_buffer = 1;
        }

        skip_buffer = 0;
        !error[0] && successes++;

        fclose(pipe);
    }

    finish_animation(&animation_th);

    sprintf(msg, "Finished. Downloaded %d/%d videos", successes, items_len);
}


void download_from_file_menu_set_videoobject(int* files_nr, int* msg_clr, VideoItem** items)
{
    char  buffer[BUF_SIZE];
    FILE* file = fopen("videos.txt", "r");


    *items    = NULL;
    *files_nr = 0;

    if (file == NULL)
    {
        *msg_clr = 2;
        sprintf(msg, "File 'videos.txt' could not be opened");

        return;
    }  

    while ( fgets(buffer, sizeof(buffer), file) )
    {
        (*files_nr)++;

        char* token;
        int   name_len,
              files_sub = (*files_nr)-1;


        *items = (VideoItem*)realloc(*items, sizeof(VideoItem) * (*files_nr));
        
        token = strtok(buffer, " ");
        strcpy( (*items)[files_sub].url, token );

        token    = strtok(NULL, " ");
        name_len = strlen(token);

        if (token[name_len-1] == '\n')
            token[name_len-1] = '\0';

        strcpy( (*items)[files_sub].name, token );
    }

    if (! *files_nr) *msg_clr = 2;
    else             *msg_clr = 3;

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

        time(&timer_start);

        return;
    }

    AnimationArgs args = {win, 5, 16};
    pthread_t     animation_th;

    int screen_width = getmaxx(win) - 10,
        skip_buffer  = 0;

    char buffer[BUF_SIZE],
         error[BUF_SIZE],
         frame[16];
    
    FILE* pipe;


    remove_element("dwn_name");
    remove_element("dwn_url");

    clear_row(win, 19);
    clear_row(win, 20);

    print_colored(win, 4, 5, 15, "Fetching URL...");

    craft_ytdlp_command(buffer, f_url, f_name);


    pipe     = popen(buffer, "r");
    error[0] = '\0';

    if (pipe == NULL)
        return;


    init_animation(&animation_th, &args);

    while ( fgets(buffer, sizeof(buffer), pipe) != NULL )
    {
        if (check_download_error(buffer, error))
            break;

        if (skip_buffer) continue;

        pthread_mutex_lock(&th_lock);
        print_colored(win, 4, 5, 15, "Downloading, please wait");
        pthread_mutex_unlock(&th_lock);

        skip_buffer = 1;
    }

    finish_animation(&animation_th);
    pclose(pipe);

    if (error[0])
    {
        strcpy(msg, error);
        row15_color = 2;
    }
    else
    {
        strcpy(msg, "Download completed");
        row15_color = 3;
    }

    f_url[0]  = '\0';
    f_name[0] = '\0';

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

            switch (*choice)
            {
                case 0: nodelay(win, TRUE); show_url_prompt  = 1; break;
                case 1: nodelay(win, TRUE); show_name_prompt = 1; break;
                case 2: download_file_link(win); break;
                case 3: nodelay(win, FALSE); return clear_return();
            }
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

                strlcpy(varbuffer, msgbuff, BUF_SIZE);

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
            snprintf(prntbuff, BUF_SIZE, "> %s", msgbuff);
        }
    }

    timer_start = timer_curr = 0;

    remove_element("dwn_empty");

    if (varbuffer[0] && !does_element_exist(element_id))
        add_element(element_id, fn);
}


void paste_legend(WINDOW* win)
{
    paste_colored(win, 1, 5, 20, "W/S   -> Navigation");
    paste_colored(win, 1, 5, 21, "Enter -> Action");
}


void menu_initial_loop_handler(WINDOW* win, int w, char* btns[], int btns_len, int* choice, int offsety, char* label)
{
    werase(win);

    print_greeting(win, w);
    print_buttons(win, btns, btns_len, choice, offsety, label);
}


void menu_final_loop_handler(WINDOW* win)
{
    box(win, 0, 0);
    wrefresh(win);
}



/* MAIN MENU HANDLERS */

void download_from_link_menu(WINDOW* win, int scr_width)
{
    char *menu[]  = { "Enter url...", "Enter filename...", "Download", "Back" };

    int choice  = 0,
        btn_len = sizeof(menu) / sizeof(char*);


    while (1)
    {
        menu_initial_loop_handler(win, scr_width, menu, btn_len, &choice, 8, "[Download from file]");

        if (timer_start)
        {
            time(&timer_curr);

            if (difftime(timer_curr, timer_start) >= 2)
            {
                nodelay(win, FALSE);

                timer_curr = timer_start = 0;
                remove_element("dwn_empty");
            }
        }

        for (int i = 0; i < elements_len; i++)
            elements[i].fn(win);

        menu_final_loop_handler(win);

        if (handle_dwnfromlink(win, menu, btn_len, &choice))
            return;
    }
}


void download_from_file_menu(WINDOW* win, int scr_width)
{
    char *menu[] = { "Download", "Edit videos", "Update videos", "Back" };

    int choicev  = 0,
        btn_len  = sizeof(menu) / sizeof(char*),
        files_nr = 0,
        msg_clr  = 1;

    int       *choice = &choicev;
    VideoItem *items  = NULL;


    download_from_file_menu_set_videoobject(&files_nr, &msg_clr, &items);

    while (1)
    {
        menu_initial_loop_handler(win, scr_width, menu, btn_len, choice, 8, "[Download from file]");
        paste_colored(win, msg_clr, 5, 15, msg);

        menu_final_loop_handler(win);

        switch (wgetch(win))
        {
            UpDownArrows;    

            case '\n':
                switch (*choice)
                {
                    case 0: 
                        if (!files_nr) break; 
                        handle_dnwfromfile(win, items, files_nr); break;
                        
                    case 1: open_path("videos.txt"); break;
                    case 2: download_from_file_menu_set_videoobject(&files_nr, &msg_clr, &items); break;
                    case 3: free(items); return;
                }
        }
    }
}


void read_config_menu(WINDOW* win, int scr_width)
{
    char *menu[]  = { "Open output directory", "Edit config", "Update config", "Back" };
    char  buffer[BUF_SIZE],
          cookies[BUF_SIZE],
          format[32];

    int vchoice = 0,
        btn_len = sizeof(menu) / sizeof(char*);
    int *choice = &vchoice;
    

    update_config_text_output(format, cookies);

    while (1)
    {
        menu_initial_loop_handler(win, scr_width, menu, btn_len, choice, 18, "[Configuration details]");
        paste_colored(win, 4, 5, 8, "config.conf");

        if (!config->success)
        {
            wattron(win, COLOR_PAIR(2));

            mvwprintw(win, 16, 5, "[ERROR] Failed reading 'config.conf'. Default values will be used");
            mvwprintw(win, 17, 5, "[ERROR] Check your configuration file");

            wattroff(win, COLOR_PAIR(2));
        }

        snprintf(buffer, BUF_SIZE, "[Output path]:    %s", config->output_path);
        mvwprintw(win, 10, 5, buffer);

        snprintf(buffer, BUF_SIZE, "[Format type]:    %s", format);
        mvwprintw(win, 11, 5, buffer);

        snprintf(buffer, BUF_SIZE, "[Audio-only ext]: %s", config->audio_ext);
        mvwprintw(win, 12, 5, buffer);

        snprintf(buffer, BUF_SIZE, "[Cookies]:        %s", cookies);
        mvwprintw(win, 13, 5, buffer);
        
        menu_final_loop_handler(win);

        switch (wgetch(win))
        {
            UpDownArrows;

            case '\n':
                switch (vchoice)
                {
                    case 0: open_path(config->output_path); break;
                    case 1: open_path("config.conf"); break;
                    case 2: init_config(); update_config_text_output(format, cookies); break;
                    case 3: return;
                }
        }
    }
}


void about_menu(WINDOW* win, int scr_width)
{
    char *menu[] = { "Open github page", "Back" };
    char *repo   = "https://github.com/vrecek/yt-dwn-ncurses";

    char repo_msg[128];

    int vchoice = 0,
        btn_len = sizeof(menu) / sizeof(char*);
    int *choice = &vchoice;
    

    snprintf(repo_msg, 128, "Repo URL:  %s", repo);

    while (1)
    {
        menu_initial_loop_handler(win, scr_width, menu, btn_len, choice, 20, "[About]");

        paste_colored(win, 1, 5, 6, "__     _________   _____                      _                 _           \n"
        "     \\ \\   / /__   __| |  __ \\                    | |               | |          \n"
        "      \\ \\_/ /   | |    | |  | | _____      ___ __ | | ___   __ _  __| | ___ _ __ \n"
        "       \\   /    | |    | |  | |/ _ \\ \\ /\\ / / '_ \\| |/ _ \\ / _` |/ _` |/ _ \\ '__|\n"
        "        | |     | |    | |__| | (_) \\ V  V /| | | | | (_) | (_| | (_| |  __/ |   \n"
        "        |_|     |_|    |_____/ \\___/ \\_/\\_/ |_| |_|_|\\___/ \\__,_|\\__,_|\\___|_|"
        );

        mvwprintw(win, 13, 5, "Author:    vrecek");
        mvwprintw(win, 14, 5, "Version:   v1.0");
        mvwprintw(win, 15, 5, repo_msg);

        menu_final_loop_handler(win);

        switch (wgetch(win))
        {
            UpDownArrows;

            case '\n':
                switch (vchoice)
                {
                    case 0: open_path(repo); break;
                    case 1: return;
                }
        }
    }
}


void exit_cleaner()
{
    free(config);
    free(elements);
}



/*  CONFIG FN  */

void init_config()
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

        return;
    }

    while ( fgets(buffer, sizeof(buffer), file) != NULL )
    {
        if (buffer[0] == '#' || buffer[0] == '\n')
            continue;

        token = strtok(buffer, " ");

        strlcpy(key, token, BUF_SIZE);

        token = strtok(NULL, " ");
        len   = strlen(token);

        if (token[len-1] == '\n')
            token[len-1] = '\0';


        if ( !strcmp(key, "output_path") )
            strlcpy(config->output_path, token, BUF_SIZE);

        else if ( !strcmp(key, "audio_ext") )
            strlcpy(config->audio_ext, token, BUF_SIZE);

        else if ( !strcmp(key, "cookies") )
            if (token[0] == '0')
                config->cookies[0] = '\0';
            else
                strlcpy(config->cookies, token, BUF_SIZE);

        else if ( !strcmp(key, "type") )
            config->type = token[0] - '0';
    }

    fclose(file);

    config->success = 1;

    return;
}



/*  CHECK AVAILABILITY FN  */

void check_availability()
{
    #ifndef __linux__
        printf("[ERROR] Available only on Linux\n");
        exit(1);
    #endif

    if (system("which yt-dlp &> /dev/null"))
    {
        printf("[ERROR] yt-dlp is not installed on the system\n");
        exit(2);
    }
}