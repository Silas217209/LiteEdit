#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct Cell {
    wchar_t ch;
    int fg;
    int bg;
    int attr;
};

struct CellBuffer {
    int width;
    int height;
    struct Cell *cells;
};

struct Global {
    struct termios orig_termios;
    struct CellBuffer buf;
};

struct Global G;

// define functions
int terminal_end();
int terminal_init();
int terminal_get_cursor_pos(int *x, int *y);
int terminal_move_cursor(int x, int y);
int terminal_get_size(int *cols, int *rows);
int cell_buffer_init(struct CellBuffer *buffer, int width, int height);
void cell_buffer_free(struct CellBuffer *buffer);
int cell_set(int x, int y, struct Cell cell);

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

// implement functions
int terminal_end() {
    // leave alternate screen
    if (write(STDOUT_FILENO, "\e[?1049l", 8) != 8) {
        errno = EIO;
        return -1;
    }
    if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &G.orig_termios) == -1) {
        errno = EIO;
        return -1;
    }
    cell_buffer_free(&G.buf);

    return 0;
}

int terminal_init() {
    if (tcgetattr(STDOUT_FILENO, &G.orig_termios) == -1) {
        errno = EIO;
        return -1;
    }
    // enable terminal raw mode
    struct termios t;

    // enter alternate screen
    if (write(STDOUT_FILENO, "\e[?1049h", 8) != 8) {
        errno = EIO;
        return -1;
    }

    t.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t.c_oflag &= ~OPOST;
    t.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= ~(CSIZE | PARENB);
    t.c_cflag |= CS8;
    t.c_cc[VMIN] = 1;
    t.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) {
        return -1;
    };

    // init buffer
    terminal_get_size(&G.buf.width, &G.buf.height);
    G.buf.cells = malloc((G.buf.width * G.buf.height) * sizeof(struct Cell));
    if (!G.buf.cells) {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}

int terminal_get_cursor_pos(int *x, int *y) {
    char buf[32];
    uint i = 0;

    if (write(STDOUT_FILENO, "\e[6n", 4) != 4) {
        errno = EIO;
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            errno = EIO;
            return -1;
        }

        if (buf[i] == 'R')
            break;
        i++;
    }

    if (buf[0] != '\e' || buf[1] != '[') {
        errno = EINVAL;
        return -1;
    }
    if (sscanf(&buf[2], "%d;%d", x, y) != 2) {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

int terminal_move_cursor(int x, int y) {
    char b[12];
    int len_b = sprintf(b, "\e[%d;%dH", x, y);

    if (write(STDOUT_FILENO, b, len_b) != len_b) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int terminal_get_size(int *cols, int *rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        errno = EIO;
        return -1;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

int cell_buffer_init(struct CellBuffer *buffer, int width, int height) {
    buffer->width = width;
    buffer->height = height;

    buffer->cells = malloc(width * height * sizeof(struct Cell));
    if (!buffer->cells) {
        errno = ENOMEM;
        return -1;
    }

    for (int i = 0; i < width * height; i++) {
        buffer->cells[i].ch = L' ';
        buffer->cells[i].fg = 0xFFFFFF;
        buffer->cells[i].bg = 0x000000;
        buffer->cells[i].attr = 0;
    }

    return 0;
}

void cell_buffer_free(struct CellBuffer *buffer) {
    if (buffer->cells) {
        free(buffer->cells);
        buffer->cells = NULL;
    }

    buffer->width = 0;
    buffer->height = 0;
}

int terminal_refresh() {
    int curr_fg = 0xFFFFFF;
    int curr_bg = 0x000000;
    int attr = 0b000000;

    char buf[100 * G.buf.width * G.buf.height];
    int len_buf = 0;

    len_buf += sprintf(&buf[len_buf], "\e[%d;%dH", 0, 0);

    len_buf +=
            sprintf(&buf[len_buf], "\e[38;2;%d;%d;%dm\e[48;2;%d;%d;%dm", (curr_fg >> 16) & 0xFF, (curr_fg >> 8) & 0xFF,
                    curr_fg & 0xFF, (curr_bg >> 16) & 0xFF, (curr_bg >> 8) & 0xFF, curr_bg & 0xFF);

    write(STDOUT_FILENO, buf, len_buf);

    int curr_row = 0;
    int curr_col = 0;

    for (int index = 0; index < G.buf.width * G.buf.height; index++) {
        int row = index / G.buf.width;
        int col = index % G.buf.height;

        struct Cell cell = G.buf.cells[index];

        if (row != curr_row) {
            row = curr_row;
            len_buf += sprintf(&buf[len_buf], "\e[%d;%dH", 0, row);
        }

        if (cell.fg != curr_fg) {
            len_buf += sprintf(&buf[len_buf], "\e[38;2;%d;%d;%dm", (cell.fg >> 16) & 0xFF, (cell.fg >> 8) & 0xFF,
                               cell.fg & 0xFF);
        }
        if (cell.bg != curr_bg) {
            len_buf += sprintf(&buf[len_buf], "\e[38;2;%d;%d;%dm", (cell.bg >> 16) & 0xFF, (cell.bg >> 8) & 0xFF,
                               cell.bg & 0xFF);
        }

        int cell_bold = (cell.attr & BOLD) != 0;
        int attr_bold = (attr & BOLD) != 0;

        if (cell_bold != attr_bold) {
            if (!cell_bold) {
                strcpy(&buf[len_buf], "\e[22m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[1m");
                len_buf += 4;
            }
        }

        int cell_italic = (cell.attr & ITALIC) != 0;
        int attr_italic = (attr & ITALIC) != 0;

        if (cell_italic != attr_italic) {
            if (!cell_italic) {
                strcpy(&buf[len_buf], "\e[23m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[3m");
                len_buf += 4;
            }
        }

        int cell_underline = (cell.attr & UNDERLINE) != 0;
        int attr_underline = (attr & UNDERLINE) != 0;

        if (cell_underline != attr_underline) {
            if (!cell_underline) {
                strcpy(&buf[len_buf], "\e[24m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[4m");
                len_buf += 4;
            }
        }

        int cell_blink = (cell.attr & BLINK) != 0;
        int attr_blink = (attr & BLINK) != 0;

        if (cell_blink != attr_blink) {
            if (!cell_blink) {
                strcpy(&buf[len_buf], "\e[25m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[5m");
                len_buf += 4;
            }
        }

        int cell_inverse = (cell.attr & INVERSE) != 0;
        int attr_inverse = (attr & INVERSE) != 0;

        if (cell_inverse != attr_inverse) {
            if (!cell_inverse) {
                strcpy(&buf[len_buf], "\e[27m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[7m");
                len_buf += 4;
            }
        }

        int cell_strikethrough = (cell.attr & STRIKETHROUGH) != 0;
        int attr_strikethrough = (attr & STRIKETHROUGH) != 0;

        if (cell_strikethrough != attr_strikethrough) {
            if (!cell_strikethrough) {
                strcpy(&buf[len_buf], "\e[29m");
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[9m");
                len_buf += 4;
            }
        }

        char temp_buf[MB_CUR_MAX];

        size_t len = wcstombs(temp_buf, &cell.ch, MB_CUR_MAX);
        if (len == (size_t) -1) {
            errno = EILSEQ;
            return -1;
        }

        strncpy(&buf[len_buf], temp_buf, len);
    }

    write(STDOUT_FILENO, buf, len_buf);

    return 0;
}

int cell_set(int x, int y, struct Cell cell) {
    if (x > G.buf.width || x < 0) {
        errno = ERANGE;
        return -1;
    }

    if (y > G.buf.height || y < 0) {
        errno = ERANGE;
        return -1;
    }

    G.buf.cells[y * G.buf.width + x] = cell;

    return 0;
}

int terminal_read_input() {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
        return ch;
    }
    return -1;
}
