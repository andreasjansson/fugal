// number of rows
#define NROW 20

// number of columns
#define NCOL 40

// ticks per minute
#define TPM (120 * 4)

// in the MESSAGE_NOTE message, the number of ticks before
// noteoff is sent
#define NOTE_DURATION 4

// the ALSA midi client name
#define MIDI_CLIENT_NAME "Fugal"

// there's only one save file, rename it if you want to
// save multiple fugues.
#define SAVE_FILE_NAME  "fugal.save.tpl"

// keyboard commands
#define CMD_MOVE_UP     KEY_UP
#define CMD_MOVE_RIGHT  KEY_RIGHT
#define CMD_MOVE_DOWN   KEY_DOWN
#define CMD_MOVE_LEFT   KEY_LEFT
#define CMD_PUT_PATH    ' '
#define CMD_DROP_BALL   '\n'
#define CMD_SET_NOTE    'n'
#define CMD_DELETE      KEY_BACKSPACE
#define CMD_SAVE        's'
#define CMD_LOAD        'l'
#define CMD_QUIT        'q'

// messages should have NROW items
message_t messages[] = {
    // label            type            chan   byte1    byte2
    {"G5",              MESSAGE_NOTE,   0,     79,      127},
    {"F#5",             MESSAGE_NOTE,   0,     78,      127},
    {"F5",              MESSAGE_NOTE,   0,     77,      127},
    {"E5",              MESSAGE_NOTE,   0,     76,      127},
    {"D#5",             MESSAGE_NOTE,   0,     75,      127},
    {"D5",              MESSAGE_NOTE,   0,     74,      127},
    {"C#5",             MESSAGE_NOTE,   0,     73,      127},
    {"C5",              MESSAGE_NOTE,   0,     72,      127},
    {"B4",              MESSAGE_NOTE,   0,     71,      127},
    {"A#4",             MESSAGE_NOTE,   0,     70,      127},
    {"A4",              MESSAGE_NOTE,   0,     69,      127},
    {"G#4",             MESSAGE_NOTE,   0,     68,      127},
    {"G4",              MESSAGE_NOTE,   0,     67,      127},
    {"F#4",             MESSAGE_NOTE,   0,     66,      127},
    {"F4",              MESSAGE_NOTE,   0,     65,      127},
    {"E4",              MESSAGE_NOTE,   0,     64,      127},
    {"D#4",             MESSAGE_NOTE,   0,     63,      127},
    {"D4",              MESSAGE_NOTE,   0,     62,      127},
    {"C#4",             MESSAGE_NOTE,   0,     61,      127},
    {"C4",              MESSAGE_NOTE,   0,     60,      127},
};
