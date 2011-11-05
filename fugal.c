#include <ncurses.h>
#include <stdlib.h>

#define NROW 20
#define NCOL 20

#define MOVE_UP         KEY_UP
#define MOVE_RIGHT      KEY_RIGHT
#define MOVE_DOWN       KEY_DOWN
#define MOVE_LEFT       KEY_LEFT
#define PUT_PATH        ' '
#define DROP_UP         'e'
#define DROP_RIGHT      'f'
#define DROP_DOWN       'c'
#define DROP_LEFT       's'
#define SET_NOTE        KEY_RETURN
#define DELETE_CELL     KEY_BACKSPACE

#define UP              0001
#define UPRIGHT         0002
#define RIGHT           0004
#define DOWNRIGHT       0010
#define DOWN            0011
#define DOWNLEFT        0012
#define LEFT            0014
#define UPLEFT          0100
#define HORIZONTAL      0101
#define VERTICAL        0102
#define PATH            0104
#define NOTE            0108

#define COLOUR_MATRIX   1
#define COLOUR_BALL     2
#define COLOUR_CURSOR   3
#define COLOUR_CURSORBALL 4

int matrix[NROW][NCOL];
int cursor_x, cursor_y, ball_x, ball_y, ball_direction;

void init_curses()
{
    initscr();
    start_color();
    init_pair(COLOUR_MATRIX, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOUR_BALL, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOUR_CURSOR, COLOR_BLUE, COLOR_WHITE);
    init_pair(COLOUR_CURSORBALL, COLOR_GREEN, COLOR_WHITE);
    raw();
    keypad(stdscr, TRUE);
    noecho();
}

void end_curses()
{
    endwin();
}

void init_matrix()
{
    int i, j;
    for(i = 0; i < NROW; i ++)
        for(j = 0; j < NCOL; j ++)
            matrix[i][j] = 0;
}

int symbol_for_cell(int cell)
{
    if(cell & (UP | RIGHT | DOWN | LEFT))
        return ACS_PLUS;

    if(cell & (UP | RIGHT | DOWN))
        return ACS_LTEE;

    if(cell & (UP | RIGHT | LEFT))
        return ACS_BTEE;

    if(cell & (UP | DOWN | LEFT))
        return ACS_RTEE;

    if(cell & (RIGHT | DOWN | LEFT))
        return ACS_TTEE;

    if(cell & (UP | LEFT))
        return ACS_LRCORNER;

    if(cell & (UP | DOWN))
        return ACS_VLINE;

    if(cell & (UP | RIGHT))
        return ACS_LLCORNER;

    if(cell & (RIGHT | DOWN))
        return ACS_ULCORNER;

    if(cell & (RIGHT | LEFT))
        return ACS_HLINE;

    if(cell & (DOWN | LEFT))
        return ACS_URCORNER;

    if(cell & PATH)
        return BULLET;
}

void redraw()
{
    int i, j, cell, colour, symbol;
    for(y = 0; y < NROW; y ++) {
        for(x = 0; x < NCOL; x ++) {
            if(y == cursor_y && x == cursor_x &&
               x == ball_x && y == ball_y)
                colour = COLOUR_CURSORBALL;
            else if(y == cursor_y && x == cursor_x)
                colour = COLOUR_CURSOR;
            else if(y == ball_y && x == ball_x)
                colour = COLOUR_BALL;
            else
                colour = COLOUR_MATRIX;
            attron(COLOR_PAIR(colour));
                
            cell = matrix[x][y];
            symbol = symbol_for_cell(cell);
        }
    }
}

void set_position(int new_cursor_x, int new_cursor_y)
{
    if(new_cursor_x < 0)
        new_cursor_x = 0;
    else if(new_cursor_x >= NCOL)
        new_cursor_x = NCOL - 1;
    if(new_cursor_y < 0)
        new_cursor_y = 0;
    else if(new_cursor_y >= NROW)
        new_cursor_y = NROW - 1;

    cursor_x = new_cursor_x;
    cursor_y = new_cursor_y;

    redraw();
}

void clear(int x, int y)
{
    matrix[x][y] = 0;
    if(x < NCOL - 1)
        matrix[x + 1][y] &= ~LEFT;
    if(x > 0)
        matrix[x - 1][y] &= ~RIGHT;
    if(y < NROW - 1)
        matrix[x][y + 1] &= ~UP;
    if(y > 0)
        matrix[x][y - 1] &= ~DOWN;
}

void put_path(int x, int y)
{
    int cell = PATH;

    if(x < NCOL - 1 && matrix[x + 1][y] & PATH) {
        matrix[x][y] |= RIGHT;
        matrix[x + 1][y] |= LEFT;
    }
    if(x > 0 && matrix[x - 1][y] & PATH) {
        matrix[x][y] |= LEFT;
        matrix[x - 1][y] |= RIGHT;
    }
    if(y < NROW - 1 && matrix[x][y + 1] & PATH) {
        matrix[x][y] |= DOWN;
        matrix[x][y + 1] |= UP;
    }
    if(y > 0 && matrix[x][y - 1] & PATH) {
        matrix[x][y] |= UP;
        matrix[x][y - 1] |= DOWN;
    }

    matrix[x][y] = cell;
}


int main()
{
    init_curses();
    init_matrix();

    int ch;
    while(ch != 'q') {
        ch = getch();
        switch(ch) {
        case MOVE_UP:
            set_position(cursor_x, cursor_y - 1);
            break;
        case MOVE_RIGHT:
            set_position(cursor_x + 1, cursor_y);
            break;
        case MOVE_DOWN:
            set_position(cursor_x, cursor_y + 1);
            break;
        case MOVE_LEFT:
            set_position(cursor_x - 1, cursor_y);
            break;

        case PUT_PATH:
            put_path(cursor_x, cursor_y);
            break;

        case SET_NOTE:
            set_note(cursor_x, cursor_y);
            break;

        case DELETE_CELL:
            clear(cursor_x, cursor_y);
            break;

        default:
            break;
        }
    }

    end_curses();

    return 0;
}

