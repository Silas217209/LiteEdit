#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#define BOLD 0b000001
#define ITALIC 0b000010
#define UNDERLINE 0b000100
#define BLINK 0b001000
#define INVERSE 0b010000
#define STRIKETHROUGH 0b100000

#define ST_INHERIT 0x1 << 31

enum editorKey {
    KEY_ARROW_LEFT = 1000,
    KEY_ARROW_RIGHT,
    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_DELETE,
    KEY_BACKSPACE,
    KEY_ENTER,
    KEY_TAB,
    KEY_ESC,
};

typedef struct {
    uint32_t fg;
    uint32_t bg;
    int attr;
} Style;

struct Cell {
    wchar_t ch;
    Style s;
};

struct CellBuffer {
    int width;
    int height;
    struct Cell *cells;
};

struct Global {
    struct termios orig_termios;
    struct CellBuffer front;
};

int terminal_end();
int terminal_init();
int terminal_get_cursor_pos(int *x, int *y);
int terminal_move_cursor(int x, int y);
int terminal_get_size(int *cols, int *rows);
int cell_buffer_init(struct CellBuffer *buffer, int width, int height);
void cell_buffer_free(struct CellBuffer *buffer);
int terminal_refresh();
int terminal_cell_set(int x, int y, struct Cell cell);
int terminal_read_input();

#endif // !TERMINAL_H
