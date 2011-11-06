// TODO: terminal resize handling (including "too small")
// TODO: config.h
// TODO: save
// TODO: row labels, also in commands, defined in config
// TODO: free all [mc]allocs ; valgrind
// TODO: double play bug when at the end of tail

#include <ncurses.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <alsa/asoundlib.h>

// put these in config
#define NROW 20
#define NCOL 40

// ticks per minute
#define TPM (120 * 4)
#define NOTE_DURATION 4 // in ticks
#define MIDI_CLIENT_NAME "Fugal"

// put these in config
#define MOVE_UP         KEY_UP
#define MOVE_RIGHT      KEY_RIGHT
#define MOVE_DOWN       KEY_DOWN
#define MOVE_LEFT       KEY_LEFT
#define PUT_PATH        ' '
#define DROP_BALL       '\n'
#define SET_NOTE        'n'
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
#define COLOUR_LABEL    000020

#define MESSAGE_NOTE    1
#define MESSAGE_NOTEOFF 2

typedef struct struct_delayed_noteoff {
    unsigned char pitch;
    unsigned char channel;
    unsigned int ticks_left;
    struct struct_delayed_noteoff *prev;
    struct struct_delayed_noteoff *next;
} delayed_noteoff_t;

typedef struct struct_ball {
    int x;
    int y;
    int direction;
    struct struct_ball *prev;
    struct struct_ball *next;
} ball_t;

typedef struct {
    char *label;
    int type;
    int channel;
    int byte1;
    int byte2;
} message_t;

// for config
message_t messages[] = {
    {"G5",              MESSAGE_NOTE,       0,     79,     127},
    {"F#5",             MESSAGE_NOTE,       0,     78,     127},
    {"F5",              MESSAGE_NOTE,       0,     77,     127},
    {"E5",              MESSAGE_NOTE,       0,     76,     127},
    {"D#5",             MESSAGE_NOTE,       0,     75,     127},
    {"D5",              MESSAGE_NOTE,       0,     74,     127},
    {"C#5",             MESSAGE_NOTE,       0,     73,     127},
    {"C5",              MESSAGE_NOTE,       0,     72,     127},
    {"B4",              MESSAGE_NOTE,       0,     71,     127},
    {"A#4",             MESSAGE_NOTE,       0,     70,     127},
    {"A4",              MESSAGE_NOTE,       0,     69,     127},
    {"G#4",             MESSAGE_NOTE,       0,     68,     127},
    {"G4",              MESSAGE_NOTE,       0,     67,     127},
    {"F#4",             MESSAGE_NOTE,       0,     66,     127},
    {"F4",              MESSAGE_NOTE,       0,     65,     127},
    {"E4",              MESSAGE_NOTE,       0,     64,     127},
    {"D#4",             MESSAGE_NOTE,       0,     63,     127},
    {"D4",              MESSAGE_NOTE,       0,     62,     127},
    {"C#4",             MESSAGE_NOTE,       0,     61,     127},
    {"C4",              MESSAGE_NOTE,       0,     60,     127},
};

int matrix[NCOL][NROW];
int cursor_x, cursor_y;
ball_t *first_ball;
ball_t *last_ball;
snd_seq_t *midi;
int midi_port_id;
delayed_noteoff_t *first_delayed_noteoff;
delayed_noteoff_t *last_delayed_noteoff;
int max_label_length;

void die(char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

void init_curses()
{
    initscr();
    start_color();
    curs_set(0);
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    init_pair(COLOUR_MATRIX, COLOR_WHITE, COLOR_BLUE);
    init_pair(COLOUR_BALL, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOUR_CURSOR, COLOR_BLUE, COLOR_WHITE);
    init_pair(COLOUR_CURSOR | COLOUR_BALL, COLOR_GREEN, COLOR_WHITE);
    init_pair(COLOUR_NOTE, COLOR_WHITE, COLOR_RED);
    init_pair(COLOUR_NOTE | COLOUR_CURSOR, COLOR_RED, COLOR_WHITE);
    init_pair(COLOUR_NOTE | COLOUR_BALL, COLOR_RED, COLOR_GREEN);
    init_pair(COLOUR_NOTE | COLOUR_BALL | COLOUR_CURSOR, COLOR_GREEN, COLOR_RED);
    init_pair(COLOUR_LABEL, COLOR_WHITE, COLOR_BLACK);
}

void end_curses()
{
    curs_set(1);
    endwin();
}

void init_matrix()
{
    int x, y;
    for(y = 0; y < NROW; y ++)
        for(x = 0; x < NCOL; x ++)
            matrix[x][y] = 0;
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

int ball_on(int x, int y)
{
    ball_t *ball;
    for(ball = first_ball; ball != NULL; ball = ball->next)
        if(ball->x == x && ball->y == y)
            return TRUE;

    return FALSE;
}

int colour_for_cell(int cell, int x, int y)
{
    int colour = 0;
    if(y == cursor_y && x == cursor_x)
        colour |= COLOUR_CURSOR;
    if(ball_on(x, y))
        colour |= COLOUR_BALL;
    if((cell & NOTE) == NOTE)
        colour |= COLOUR_NOTE;
    if(colour == 0)
        colour = COLOUR_MATRIX;
    return colour;
}

int get_max_label_length()
{
    int max_length = 0, length, i;
    for(i = 0; i < NROW; i ++) {
        length = strlen(messages[i].label);
        if(length > max_length)
            max_length = length;
    }
    return max_length;
}

void redraw()
{
    int x, y, cell, colour, symbol, max_x, max_y,
        offset_x, offset_y;

    getmaxyx(stdscr, max_y, max_x);
    offset_x = max_x / 2 - NCOL / 2;
    offset_y = max_y / 2 - NROW / 2;

    for(y = 0; y < NROW; y ++) {
        for(x = 0; x < NCOL; x ++) {
            cell = matrix[x][y];
            colour = colour_for_cell(cell, x, y);
            attron(COLOR_PAIR(colour));
            symbol = symbol_for_cell(cell);
            mvaddch(y + offset_y, x + offset_x, symbol);
        }
    }

    int label_space = offset_x - 1;
    int label_length = max_label_length > label_space ? label_space : max_label_length;
    int label_offset_x = label_space - label_length;
    char *label;
    attron(COLOR_PAIR(COLOUR_LABEL));
    for(y = 0; y < NROW; y ++) {
        label = strndup(messages[y].label, label_space);
        if(y == cursor_y)
            attron(A_BOLD);
        else
            attroff(A_BOLD);
        mvprintw(y + offset_y, label_offset_x, "%s", label);
    }
    free(label);
    
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
    matrix[x][y] |= PATH;

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

// TODO: tidy up
void add_delayed_noteoff(unsigned char pitch, unsigned char channel)
{
    delayed_noteoff_t *noteoff = calloc(1, sizeof(delayed_noteoff_t));
    noteoff->pitch = pitch;
    noteoff->channel = channel;
    noteoff->ticks_left = NOTE_DURATION;
    noteoff->prev = NULL;

    if(first_delayed_noteoff == NULL)
        first_delayed_noteoff = last_delayed_noteoff = noteoff;
    else {
        noteoff->next = first_delayed_noteoff;
        first_delayed_noteoff->prev = noteoff;
        first_delayed_noteoff = noteoff;
    }
}

// TODO: tidy up
void play_message(message_t *message)
{
    snd_seq_event_t event;
    int err;

    switch(message->type) {
    case MESSAGE_NOTE:
        if((err = snd_seq_ev_set_noteon(&event, message->channel, message->byte1,
                                        message->byte2)) < 0)
            die("Failed to set note");
        add_delayed_noteoff(message->byte1, message->channel);
        break;
        
    case MESSAGE_NOTEOFF:
        if((err = snd_seq_ev_set_noteoff(&event, message->channel, message->byte1, 0)) < 0)
            die("Failed to set note off");
        break;
        
    default:
        die("No message found. Please check your configuration.\n");
    }

    if((err = snd_seq_ev_set_direct(&event)) < 0) {
        fprintf(stderr, "Failed to set direct: %s\n", snd_strerror(err));
    }
    if((err = snd_seq_ev_set_source(&event, midi_port_id)) < 0) {
        fprintf(stderr, "Failed to set source: %s\n", snd_strerror(err));
    }
    if((err = snd_seq_ev_set_subs(&event)) < 0) {
        fprintf(stderr, "Failed to set subs: %s\n", snd_strerror(err));
    }
    if((err = snd_seq_event_output_direct(midi, &event)) < 0) {
        fprintf(stderr, "Failed to output direct: %s\n", snd_strerror(err));
    }
}

void play(int y)
{
    message_t *message = &messages[y];
    play_message(message);
}

// try going down, then right, up, left
void drop_ball(int x, int y)
{
    if((matrix[x][y] & PATH) != PATH)
        return;

    ball_t *ball;
    if(!(ball = calloc(1, sizeof(ball_t)))) {
        die("Failed to create new ball.");
    }

    ball->x = x;
    ball->y = y;
    if((matrix[x][y] & DOWN) == DOWN)
        ball->direction = DOWN;
    else if((matrix[x][y] & RIGHT) == RIGHT)
        ball->direction = RIGHT;
    else if((matrix[x][y] & UP) == UP)
        ball->direction = UP;
    else if((matrix[x][y] & LEFT) == LEFT)
        ball->direction = LEFT;
    else
        ball->direction = DOWN;

    if((matrix[x][y] & NOTE) == NOTE)
        play(y);

    if(first_ball == NULL)
        last_ball = first_ball = ball;
    else {
        ball->prev = last_ball;
        last_ball->next = ball;
        last_ball = ball;
    }
}

void get_possible_directions(ball_t *ball, int *directions, int *count)
{
    *count = 0;
    int cell = matrix[ball->x][ball->y];
    if((cell & UP) == UP)
        directions[(*count) ++] = UP;
    if((cell & RIGHT) == RIGHT)
        directions[(*count) ++] = RIGHT;
    if((cell & DOWN) == DOWN)
        directions[(*count) ++] = DOWN;
    if((cell & LEFT) == LEFT)
        directions[(*count) ++] = LEFT;
}

int get_opposite(int direction)
{
    switch(direction) {
    case UP:
        return DOWN;
    case RIGHT:
        return LEFT;
    case DOWN:
        return UP;
    case LEFT:
        return RIGHT;
    default:
        return 0;
    }
}

int is_opposite(int direction1, int direction2)
{
    return direction2 == get_opposite(direction1);
}

void filter_old_direction(int *directions, int *count, int direction)
{
    int opposite = get_opposite(direction);
    int i;
    for(i = 0; i < *count; i ++) {
        if(directions[i] == opposite) {
            directions[i] = directions[(*count) - 1];
            (*count) --;
            return;
        }
    }
}

int get_new_direction(ball_t *ball)
{
    int possible_directions[4];
    int possible_directions_count;
    get_possible_directions(ball, possible_directions, &possible_directions_count);

    filter_old_direction(possible_directions, &possible_directions_count, ball->direction);

    if(possible_directions_count == 0)
        return 0;

    if(possible_directions_count == 1)
        return possible_directions[0];

    return possible_directions[random() % possible_directions_count];
}

void increment_ball_position(ball_t *ball)
{
    switch(ball->direction) {
    case UP:
        ball->y --;
        break;
    case RIGHT:
        ball->x ++;
        break;
    case DOWN:
        ball->y ++;
        break;
    case LEFT:
        ball->x --;
        break;
    default:
        break;
    }
}

int ball_is_outside(ball_t *ball)
{
    return ball->x < 0 || ball-> y < 0 || ball->x >= NCOL || ball->y >= NROW;
}

void remove_ball(ball_t *ball)
{
    if(ball == first_ball && ball == last_ball)
        first_ball = last_ball = NULL;
    else if(ball == first_ball) {
        first_ball = ball->next;
        first_ball->prev = NULL;
    }
    else if(ball == last_ball) {
        last_ball = ball->prev;
        last_ball->next = NULL;
    }
    else {
        ball->prev->next = ball->next;
        ball->next->prev = ball->prev;
    }
    free(ball);
}

void remove_delayed_noteoff(delayed_noteoff_t *delayed_noteoff)
{
    if(delayed_noteoff == first_delayed_noteoff && delayed_noteoff == last_delayed_noteoff)
        first_delayed_noteoff = last_delayed_noteoff = NULL;
    else if(delayed_noteoff == first_delayed_noteoff) {
        first_delayed_noteoff = delayed_noteoff->next;
        first_delayed_noteoff->prev = NULL;
    }
    else if(delayed_noteoff == last_delayed_noteoff) {
        last_delayed_noteoff = delayed_noteoff->prev;
        last_delayed_noteoff->next = NULL;
    }
    else {
        delayed_noteoff->prev->next = delayed_noteoff->next;
        delayed_noteoff->next->prev = delayed_noteoff->prev;
    }
    free(delayed_noteoff);
}

// don't reverse, just fall out
void step_ball(ball_t *ball)
{
    int new_direction = get_new_direction(ball);
    if(!new_direction)
        remove_ball(ball);
    ball->direction = new_direction;
    increment_ball_position(ball);

    if(ball_is_outside(ball))
        remove_ball(ball);

    if((matrix[ball->x][ball->y] & NOTE) == NOTE)
        play(ball->y);
}

void turn_notes_off()
{
    message_t message;
    delayed_noteoff_t *noteoff;
    for(noteoff = first_delayed_noteoff; noteoff != NULL; noteoff = noteoff->next) {
        noteoff->ticks_left --;
        if(noteoff->ticks_left == 0) {
            message.type = MESSAGE_NOTEOFF;
            message.channel = noteoff->channel;
            message.byte1 = noteoff->pitch;
            play_message(&message);
            remove_delayed_noteoff(noteoff);
        }
    }
}

// TODO: getmaxyx breaks after this.
void catch_resize(int sig)
{
    clear();
}

void catch_timer(int sig)
{
    ball_t *ball;
    for(ball = first_ball; ball != NULL; ball = ball->next)
        step_ball(ball);

    turn_notes_off();

    redraw();
}

void init_timer()
{
    struct itimerval timer;
    long int usec = 60 * 1000000 / TPM;
    long int sec = usec / 1000000;
    usec = usec % 1000000;
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = usec;
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = usec;

    signal(SIGALRM, catch_timer);
    setitimer(ITIMER_REAL, &timer, NULL);
}

void init_alsa()
{
    int err;

    if((err = snd_seq_open(&midi, "default", SND_SEQ_OPEN_OUTPUT, 0)) < 0) {
        fprintf(stderr, "Error opening ALSA sequencer: %s\n", snd_strerror(err));
        exit(1);
    }

    if((err = snd_seq_set_client_name(midi, MIDI_CLIENT_NAME)) < 0) {
        fprintf(stderr, "Failed to name client: %s\n", snd_strerror(err));
        exit(1);
    }

    if((midi_port_id = snd_seq_create_simple_port(
                                                  midi, MIDI_CLIENT_NAME,
	SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,                                                  SND_SEQ_PORT_TYPE_MIDI_GM)) < 0) {
        fprintf(stderr, "Failed to create port: %s\n", snd_strerror(midi_port_id));
        exit(1);
    }

    printf("Opened sequencer handle.\n");
    printf(" Sequencer handle name: %s\n", snd_seq_name(midi));
    printf(" Client id: %d\n", snd_seq_client_id(midi));
    printf(" Port id: %d\n", midi_port_id);
}

int main()
{
    // to be reproducible
    srandom(0);

    cursor_x = NCOL / 2 - 1;
    cursor_y = NROW / 2 - 1;

    init_alsa();
    init_curses();
    signal(SIGWINCH, catch_resize);
    init_matrix();
    max_label_length = get_max_label_length();
    init_timer();
    redraw();

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

        case DROP_BALL:
            drop_ball(cursor_x, cursor_y);
            redraw();
            break;

        default:
            break;
        }
    }

    end_curses();

    exit(0);
}

