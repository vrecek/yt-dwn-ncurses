#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils_public.h"


int main() {
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
    
    char *menu[] = { "Download from link", "Download from file", "Read config", "Quit" };

    int menu_len   = sizeof(menu) / sizeof(char*),
        choice     = 0,
        term_width = getmaxx(stdscr);

    WINDOW *win    = newwin(getmaxy(stdscr), term_width, 0, 0);
    Config *config = init_config();


    init_pair(1, COLOR_BLUE, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_YELLOW, -1);


    while (choice != 'q')
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, term_width);   
        print_buttons(win, menu, menu_len, &choice, 8, "[Main Menu]");

        wrefresh(win);

        switch (wgetch(win))
        {
            case 'w': 
                choice = (choice == 0) ? menu_len-1 : choice-1; 
                break; 

            case 's':
                choice = (choice == menu_len-1) ? 0 : choice+1; 
                break; 

            case '\n':
                if ( !strcmp(menu[choice], "Quit") )
                    choice = 'q';

                else if ( !strcmp(menu[choice], "Download from link") )
                    download_from_link_menu(win, term_width);

                else if ( !strcmp(menu[choice], "Download from file") )
                    download_from_file_menu(win, term_width);

                else if ( !strcmp(menu[choice], "Read config") )
                    read_config_menu(win, term_width);
        }
    }
    

    free(config);

    delwin(win); 
    endwin(); 

    printf("\n");

    return 0;
}
