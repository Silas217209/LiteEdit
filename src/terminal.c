#include "terminal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct Global G;

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
    cell_buffer_free(&G.front);

    return 0;
}

int terminal_init() {

    if (tcgetattr(STDOUT_FILENO, &G.orig_termios) == -1) {
        perror("tcgetattr failed");
        return -1;
    }

    struct termios raw = G.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr failed");
        return -1;
    }

    if (write(STDOUT_FILENO, "\x1b[?1049h", 8) != 8) {
        perror("Failed to enter alternate screen buffer");
        return -1;
    }

    if (terminal_get_size(&G.front.width, &G.front.height) == -1) {
        perror("Failed to get terminal size");
        return -1;
    }

    G.front.cells = calloc(G.front.width * G.front.height, sizeof(struct Cell));
    if (!G.front.cells) {
        perror("Failed to allocate cell buffer");
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
    int len_b = sprintf(b, "\e[%d;%dH", y, x);

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
        buffer->cells[i].s.fg = 0xFFFFFF;
        buffer->cells[i].s.bg = 0x000000;
        buffer->cells[i].s.attr = 0;
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
    char buf[100 * G.front.width * G.front.height];
    int len_buf = 0;


    uint32_t curr_fg = 0xFFFFFF;
    uint32_t curr_bg = 0x000000;
    uint attr = 0;

    strcpy(&buf[len_buf],
           "\e[s\e[?25l\e[H"); // save current cursor position and hide the
                               // cursor and move it to the top left
    len_buf += 12;

    int curr_row = 0;
    int curr_col = 0;

    for (int index = 0; index < G.front.width * G.front.height; index++) {
        int row = index / G.front.width;
        int col = index % G.front.width;

        struct Cell cell = G.front.cells[index];

        if (row != curr_row) {
            curr_row = row;
            curr_col = 0;
            strcpy(&buf[len_buf], "\r\n");
            len_buf += 2;
        }

        if (col != curr_col + 1) {
            len_buf += sprintf(&buf[len_buf], "\e[%d;%dH", row, col);
        }

        if (cell.s.fg != curr_fg && (cell.s.fg & ST_INHERIT) == 0) {
            len_buf += sprintf(&buf[len_buf], "\e[38;2;%d;%d;%dm",
                               (cell.s.fg >> 16) & 0xFF,
                               (cell.s.fg >> 8) & 0xFF, // set foreground color
                               cell.s.fg & 0xFF);
            curr_fg = cell.s.fg;
        }
        if (cell.s.bg != curr_bg && (cell.s.bg & ST_INHERIT) == 0) {
            len_buf += sprintf(&buf[len_buf], "\e[48;2;%d;%d;%dm",
                               (cell.s.bg >> 16) & 0xFF,
                               (cell.s.bg >> 8) & 0xFF, // set background color
                               cell.s.bg & 0xFF);
            curr_bg = cell.s.bg;
        }

        int cell_bold = (cell.s.attr & BOLD) != 0;
        int attr_bold = (attr & BOLD) != 0;

        if (cell_bold != attr_bold) {
            if (!cell_bold) {
                strcpy(&buf[len_buf], "\e[22m"); // bold off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[1m"); // bold on
                len_buf += 4;
            }

            attr ^= BOLD;
        }

        int cell_italic = (cell.s.attr & ITALIC) != 0;
        int attr_italic = (attr & ITALIC) != 0;

        if (cell_italic != attr_italic) {
            if (!cell_italic) {
                strcpy(&buf[len_buf], "\e[23m"); // italic off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[3m"); // italic on
                len_buf += 4;
            }
            attr ^= ITALIC;
        }

        int cell_underline = (cell.s.attr & UNDERLINE) != 0;
        int attr_underline = (attr & UNDERLINE) != 0;

        if (cell_underline != attr_underline) {
            if (!cell_underline) {
                strcpy(&buf[len_buf], "\e[24m"); // underline off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[4m"); // underline on
                len_buf += 4;
            }
            attr ^= UNDERLINE;
        }

        int cell_blink = (cell.s.attr & BLINK) != 0;
        int attr_blink = (attr & BLINK) != 0;

        if (cell_blink != attr_blink) {
            if (!cell_blink) {
                strcpy(&buf[len_buf], "\e[25m"); // blink off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[5m"); // blink on
                len_buf += 4;
            }
        }

        int cell_inverse = (cell.s.attr & INVERSE) != 0;
        int attr_inverse = (attr & INVERSE) != 0;

        if (cell_inverse != attr_inverse) {
            if (!cell_inverse) {
                strcpy(&buf[len_buf], "\e[27m"); // inverso off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[7m"); // inverse on
                len_buf += 4;
            }

            attr ^= INVERSE;
        }

        int cell_strikethrough = (cell.s.attr & STRIKETHROUGH) != 0;
        int attr_strikethrough = (attr & STRIKETHROUGH) != 0;
        if (cell_strikethrough != attr_strikethrough) {
            if (!cell_strikethrough) {
                strcpy(&buf[len_buf], "\e[29m"); // strikethrough off
                len_buf += 5;
            } else {
                strcpy(&buf[len_buf], "\e[9m"); // striketrhough on
                len_buf += 4;
            }
            attr ^= STRIKETHROUGH;
        }


        char temp_buf[MB_CUR_MAX];
        size_t len = wcstombs(temp_buf, &cell.ch, MB_CUR_MAX);
        if (len == (size_t) -1) {
            errno = EILSEQ;
            return -1;
        }

        strncpy(&buf[len_buf], temp_buf, len);
        len_buf += len;
    }

    strcpy(&buf[len_buf],
           "\e[u\e[?25h"); // restore cursor position and show the cursor
    len_buf += 9;

    write(STDOUT_FILENO, buf, len_buf);

    return 0;
}

int terminal_cell_set(int x, int y, struct Cell cell) {
    if (x > G.front.width || x < 0) {
        errno = ERANGE;
        return -1;
    }

    if (y > G.front.height || y < 0) {
        errno = ERANGE;
        return -1;
    }

    G.front.cells[y * G.front.width + x] = cell;

    return 0;
}

int terminal_read_input() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            errno = EIO;
            return -1;
        }
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1':
                            return KEY_HOME;
                        case '3':
                            return KEY_DELETE;
                        case '4':
                            return KEY_END;
                        case '5':
                            return KEY_PAGE_UP;
                        case '6':
                            return KEY_PAGE_DOWN;
                        case '7':
                            return KEY_HOME;
                        case '8':
                            return KEY_END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A':
                        return KEY_ARROW_UP;
                    case 'B':
                        return KEY_ARROW_DOWN;
                    case 'C':
                        return KEY_ARROW_RIGHT;
                    case 'D':
                        return KEY_ARROW_LEFT;
                    case 'H':
                        return KEY_HOME;
                    case 'F':
                        return KEY_END;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H':
                    return KEY_HOME;
                case 'F':
                    return KEY_END;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}
