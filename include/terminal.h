#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#define CTRL(key) ((key) & 0x1F)
#define SHIFT(key) (0x10 | (key))
#define ALT(key) (0x20 | (key))
#define CTRL(key) (0x40 | (key))
#define META(key) (0x80 | (key))


#define BOLD 0b000001
#define ITALIC 0b000010
#define UNDERLINE 0b000100
#define BLINK 0b001000
#define INVERSE 0b010000
#define STRIKETHROUGH 0b100000

struct t_cell {
    wchar_t ch;
    int fg;
    int bg;
    int attr;
};

struct t_cell_buffer {
    int width;
    int height;
    struct t_cell *cells;
};

struct t_global {
    struct termios orig_termios;
    struct t_cell_buffer buf;
};

struct t_global G;

// define functions
int t_end();
int t_init();
int t_get_cursor_pos(int *x, int *y);
int t_move_cursor(int x, int y);
int t_get_term_size(int *cols, int *rows);
int init_cell_buffer(struct t_cell_buffer *buffer, int width, int height);
void free_cell_buffer(struct t_cell_buffer *buffer);

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

// implement functions
int t_end() {
    // leave alternate screen
    if (write(STDOUT_FILENO, "\e[?1049l", 8) != 8) {
        errno = EIO;
        return -1;
    }
    if (tcsetattr(STDOUT_FILENO, TCSAFLUSH, &G.orig_termios) == -1) {
        errno = EIO;
        return -1;
    }
    free_cell_buffer(&G.buf);

    return 0;
}

int t_init() {
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

    // init buffer
    t_get_term_size(&G.buf.width, &G.buf.height);
    G.buf.cells = malloc((G.buf.width * G.buf.height) * sizeof(struct t_cell));
    if (!G.buf.cells) {
        errno = ENOMEM;
        return -1;
    }

    return 0;
}

int t_get_cursor_pos(int *x, int *y) {
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

int t_move_cursor(int x, int y) {
    char b[12];
    int len_b = sprintf(b, "\e[%d;%dH", x, y);

    if (write(STDOUT_FILENO, b, len_b) != len_b) {
        errno = EIO;
        return -1;
    }

    return 0;
}

int t_get_term_size(int *cols, int *rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        errno = EIO;
        return -1;
    }

    *cols = ws.ws_col;
    *rows = ws.ws_row;

    return 0;
}

int init_cell_buffer(struct t_cell_buffer *buffer, int width, int height) {
    buffer->width = width;
    buffer->height = height;

    buffer->cells = malloc(width * height * sizeof(struct t_cell));
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

void free_cell_buffer(struct t_cell_buffer *buffer) {
    if (buffer->cells) {
        free(buffer->cells);
        buffer->cells = NULL;
    }

    buffer->width = 0;
    buffer->height = 0;
}

int refresh() {
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

        struct t_cell cell = G.buf.cells[index];

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

int set_cell(int x, int y, struct t_cell cell) {
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


int get_keypress() {
    int c;
    if (read(STDIN_FILENO, &c, 1) == -1) {
        errno = EIO;
        return -1;
    }

    if (c == '\e') { // If the first character is an escape character
        int buf[32];
        int index = 0;

        // Read the next character
        if (read(STDIN_FILENO, &buf[index], 1) == -1) {
            return '\e'; // If there's nothing else, return the escape character itself
        }

        if (buf[0] == '\e') {
            return '\e'; // Handle case where two escape characters are read in sequence
        }

        // If the character after ESC is not '[', it's an Alt-keypress
        if (buf[0] != '[') {
            return ALT(buf[0]); // Use the ALT macro to encode the Alt+keypress
        }

        // Start reading the rest of the sequence
        index++;
        while (index < sizeof(buf)) {
            if (read(STDIN_FILENO, &buf[index], 1) == -1) {
                if (index == 1) {
                    return ALT('[');
                }
                return -1; // Return an error if reading fails
            }

            // Check if we've hit the end of the sequence
            if (buf[index] >= 'A' && buf[index] <= 'Z') {
                break; // For sequences like <esc>[A (arrow keys, etc.)
            }

            if (buf[index] == '~') {
                break; // For sequences like <esc>[3~ (Delete key, etc.)
            }

            index++;
        }

        // Now we have a complete sequence in buf
        if (buf[index] == '~') {
            // Handle VT sequences with ~ at the end
            int keycode = buf[1] - '0'; // Assuming simple single-digit keycodes for simplicity
            return keycode; // This would return something like 3 for <esc>[3~ (Delete)
        } else {
            // Handle xterm sequences (e.g., arrow keys)
            int keycode = buf[index]; // The terminating character (e.g., 'A' for Up)
            return keycode; // This would return something like 'A' for <esc>[A (Up arrow)
        }
    }

    return c; // If it's a regular character, return it as-is
}
