#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


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
    

    int    *term_size = get_window_size(stdscr);
    char   *menu[]    = { "Download from link", "Download from file", "Quit" };
    WINDOW *win       = newwin(term_size[1], term_size[0], 0, 0);
    int menu_len = sizeof(menu) / sizeof(char*),
        choice   = 0;


    nodelay(win, TRUE);

    init_pair(1, COLOR_BLUE, -1);
    init_pair(2, COLOR_RED, -1);
    init_pair(3, COLOR_GREEN, -1);
    init_pair(4, COLOR_YELLOW, -1);


    while (choice != 'q')
    {
        werase(win);

        box(win, 0, 0);

        print_greeting(win, term_size[0]);   
        print_buttons(win, menu, menu_len, &choice, 5);

        wrefresh(win);

        handle_mainmenu_input(win, menu, menu_len, &choice, term_size[0]);
    }


    delwin(win); 
    endwin(); 

    free(term_size);

    return 0;
}
