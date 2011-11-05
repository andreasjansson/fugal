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
#define SET_NOTE        KEY_ENTER
#define DELETE_CELL     KEY_BACKSPACE

#define UP              000001
#define UPRIGHT         000002
#define RIGHT           000004
#define DOWNRIGHT       000010
#define DOWN            000020
#define DOWNLEFT        000040
#define LEFT            000100
#define UPLEFT          000200
#define HORIZONTAL      000400
#define VERTICAL        001000
#define PATH            002000
#define NOTE            004000

#define COLOUR_MATRIX   000001
#define COLOUR_BALL     000002
#define COLOUR_CURSOR   000004
#define COLOUR_NOTE     000010

int matrix[NROW][NCOL];
int cursor_x, cursor_y, ball_x, ball_y, ball_direction;

void init_curses()
{
    initscr();
    start_color();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    init_pair(COLOUR_MATRIX, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOUR_BALL, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOUR_CURSOR, COLOR_BLUE, COLOR_WHITE);
    init_pair(COLOUR_CURSOR | COLOUR_BALL, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOUR_NOTE, COLOR_RED, COLOR_BLUE);
    init_pair(COLOUR_NOTE | COLOUR_CURSOR, COLOR_WHITE, COLOR_RED);
    init_pair(COLOUR_NOTE | COLOUR_BALL, COLOR_RED, COLOR_GREEN);
    init_pair(COLOUR_NOTE | COLOUR_BALL | COLOUR_CURSOR, COLOR_GREEN, COLOR_RED);
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
    if((cell & (UP | RIGHT | DOWN | LEFT)) == (UP | RIGHT | DOWN | LEFT))
        return ACS_PLUS;

    if((cell & (UP | RIGHT | DOWN)) == (UP | RIGHT | DOWN))
        return ACS_LTEE;

    if((cell & (UP | RIGHT | LEFT)) == (UP | RIGHT | LEFT))
        return ACS_BTEE;

    if((cell & (UP | DOWN | LEFT)) == (UP | DOWN | LEFT))
        return ACS_RTEE;

    if((cell & (RIGHT | DOWN | LEFT)) == (RIGHT | DOWN | LEFT))
        return ACS_TTEE;

    if((cell & (UP | LEFT)) == (UP | LEFT))
        return ACS_LRCORNER;

    if((cell & (UP | DOWN)) == (UP | DOWN))
        return ACS_VLINE;

    if((cell & (UP | RIGHT)) == (UP | RIGHT))
        return ACS_LLCORNER;

    if((cell & (RIGHT | DOWN)) == (RIGHT | DOWN))
        return ACS_ULCORNER;

    if((cell & (RIGHT | LEFT)) == (RIGHT | LEFT))
        return ACS_HLINE;

    if((cell & (DOWN | LEFT)) == (DOWN | LEFT))
        return ACS_URCORNER;

    if((cell & LEFT) == LEFT)
        return ACS_HLINE;

    if((cell & RIGHT) == RIGHT)
        return ACS_HLINE;

    if((cell & UP) == UP)
        return ACS_VLINE;

    if((cell & DOWN) == DOWN)
        return ACS_VLINE;

    if((cell & PATH) == PATH)
        return ACS_BULLET;

    return ' ';
}

void set_note(int x, int y)
{
    matrix[x][y] |= NOTE;
}

int colour_for_cell(int cell, int x, int y)
{
    int colour = 0;
    if(y == cursor_y && x == cursor_x)
        colour |= COLOUR_CURSOR;
    if(y == ball_y && x == ball_x)
        colour |= COLOUR_BALL;
    if((cell & NOTE) == NOTE)
        colour |= COLOUR_NOTE;
    if(colour == 0)
        colour = COLOUR_MATRIX;
    return colour;
}

void redraw()
{
    int x, y, cell, colour, symbol;
    for(y = 0; y < NROW; y ++) {
        for(x = 0; x < NCOL; x ++) {
            cell = matrix[x][y];
            colour = colour_for_cell(cell, x, y);
            attron(COLOR_PAIR(colour));
            symbol = symbol_for_cell(cell);
            mvaddch(y, x, symbol);
        }
    }

    refresh();
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
}

void clear_cell(int x, int y)
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
    matrix[x][y] = PATH;

    if(x < NCOL - 1 && (matrix[x + 1][y] & PATH) == PATH) {
        matrix[x][y] |= RIGHT;
        matrix[x + 1][y] |= LEFT;
    }
    if(x > 0 && (matrix[x - 1][y] & PATH) == PATH) {
        matrix[x][y] |= LEFT;
        matrix[x - 1][y] |= RIGHT;
    }
    if(y < NROW - 1 && (matrix[x][y + 1] & PATH) == PATH) {
        matrix[x][y] |= DOWN;
        matrix[x][y + 1] |= UP;
    }
    if(y > 0 && (matrix[x][y - 1] & PATH) == PATH) {
        matrix[x][y] |= UP;
        matrix[x][y - 1] |= DOWN;
    }
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
            redraw();
            break;
        case MOVE_RIGHT:
            set_position(cursor_x + 1, cursor_y);
            redraw();
            break;
        case MOVE_DOWN:
            set_position(cursor_x, cursor_y + 1);
            redraw();
            break;
        case MOVE_LEFT:
            set_position(cursor_x - 1, cursor_y);
            redraw();
            break;

        case PUT_PATH:
            put_path(cursor_x, cursor_y);
            redraw();
            break;

        case SET_NOTE:
            set_note(cursor_x, cursor_y);
            redraw();
            break;

        case DELETE_CELL:
            clear_cell(cursor_x, cursor_y);
            redraw();
            break;

        default:
            break;
        }
    }

    end_curses();

    return 0;
}

