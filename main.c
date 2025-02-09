#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils_public.h"


int main() {
    int status = check_availability();

    if (status)
    {
        printf("Press any key to exit\n");
        scanf("%c");

        return status;
    }

    initscr(); 
    cbreak();
    noecho();
    curs_set(0);
    
    if (!has_colors())
    {
        endwin();
        printf("Your terminal does not support colors\n");

        return 1;
    }

    start_color();
    use_default_colors();
    init_config();
    
    char *menu[] = { "Download from url", "Download from file", "Read config", "About", "Quit" };

    int btn_len    = sizeof(menu) / sizeof(char*),
        vchoice    = 0,
        term_width = getmaxx(stdscr);

    int *choice = &vchoice;

    WINDOW *win = newwin(getmaxy(stdscr), term_width, 0, 0);

    init_pair(1, COLOR_BLUE, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_YELLOW, -1);


    while (vchoice != 'q')
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, term_width);   
        print_buttons(win, menu, btn_len, choice, 8, "[Main Menu]");
        paste_legend(win);

        wrefresh(win);

        switch (wgetch(win))
        {
            UpDownArrows;

            case '\n':
                switch (vchoice)
                {
                    case 0: download_from_link_menu(win, term_width); break;
                    case 1: download_from_file_menu(win, term_width); break;
                    case 2: read_config_menu(win, term_width);  break;
                    case 3: about_menu(win, term_width); break;
                    case 4: vchoice = 'q'; break;
                }
        }
    }
    
    exit_cleaner();
    delwin(win); 
    endwin(); 

    return 0;
}
