#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "terminal.h"

/*
enum color { FG = 1, BG };

typedef enum {
    HORIZONTAL,
    VERTICAL,
} direction;

typedef struct {
    size_t size;
    char *chars;
} erow;

struct editorConfig {
    int cx, cy;
    int row_offset;
    int col_offset;
    int screen_cols;
    int screen_rows;
    int num_rows;
    erow *row;
};

struct editorConfig E;

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() { t_end(); }

void enableRawMode() {
    setlocale(LC_ALL, "");
    if (t_init() != 0)
        die("tb_init");
    atexit(disableRawMode);
}


void editorDrawLines() {
    for (int y = 0; y < E.screen_rows; y++) {
        if (y + E.row_offset < E.num_rows) {
            size_t len = E.row[y + E.row_offset].size - E.col_offset;
            if (len < 0)
                len = 0;
            if (len > E.screen_cols)
                len = E.screen_cols;

            for (int x = 0; x < len; x++) {
                int rx = x + E.col_offset;
                tb_set_cell(x, y, E.row[y + E.row_offset].chars[rx], TB_DEFAULT, TB_DEFAULT);
            }

            E.row[y + E.row_offset].chars[E.col_offset + len] = '\0';
            tb_print(0, y, TB_WHITE, TB_DEFAULT, &E.row[y + E.row_offset].chars[E.col_offset]);

            continue;
        }
        tb_print(0, y, TB_WHITE, TB_DEFAULT, "~");
    }
}


void editorMoveCursor(direction dir, int value) {
    erow *row = (E.cy >= E.num_rows) ? NULL : &E.row[E.cy];
    switch (dir) {
        case HORIZONTAL:
            if (E.cx > 0) {
                if (row && E.cx < row->size)
                    E.cx += value;
            } else {
                E.cx += value;
            }
            break;
        case VERTICAL:
            E.cy += value;
            E.cy = clamp(E.cy, 0, E.num_rows);
            break;
    }

    row = (E.cy >= E.num_rows) ? NULL : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
}

void editorScroll() {

    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + E.screen_rows) {
        E.row_offset = E.cy - E.screen_rows + 1;
    }

    if (E.cx < E.col_offset) {
        E.col_offset = E.cx;
    }
    if (E.cx >= E.col_offset + E.screen_cols) {
        E.col_offset = E.cx - E.screen_cols + 1;
    }
}

void editorRefreshScreen() {
    editorScroll();
    tb_clear();
    editorDrawLines();
    tb_present();
    tb_set_cursor(E.cx, E.cy);
}


void editorProcessKeypress(uint16_t key, uint32_t ch) {
    switch (ch) {
        case 'q':
            exit(0);
        case 'h':
            editorMoveCursor(HORIZONTAL, -1);
            return;
        case 'j':
            editorMoveCursor(VERTICAL, 1);
            return;
        case 'k':
            editorMoveCursor(VERTICAL, -1);
            return;
        case 'l':
            editorMoveCursor(HORIZONTAL, 1);
            return;
        default:;
    }

    switch (key) {
        case ctrl('u'):
        case TB_KEY_PGUP:
            editorMoveCursor(VERTICAL, -34);
            break;
        case ctrl('d'):
        case TB_KEY_PGDN:
            editorMoveCursor(VERTICAL, 34);
            break;
        default:;
    }
}

void editorReadEvent() {
    struct tb_event event;
    tb_peek_event(&event, 10);


    switch (event.type) {
        case TB_EVENT_RESIZE:
        case TB_EVENT_MOUSE:
        case TB_EVENT_KEY:
            editorProcessKeypress(event.key, event.ch);
    }
}

void editorAppenRow(char *line, size_t len) {
    E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));

    int index = E.num_rows;
    E.row[index].size = len;
    E.row[index].chars = malloc(len + 1);

    strncpy(E.row[index].chars, line, len);
    E.num_rows++;
}

void editorOpen(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp)
        die("fopen");

    ssize_t linelen;
    size_t len = 0;
    char *line = NULL;

    while ((linelen = getline(&line, &len, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppenRow(line, linelen);
    }

    free(line);
    fclose(fp);
}


void initEditor() {
    E.screen_cols = tb_width();
    E.screen_rows = tb_height();
    E.cx = 0;
    E.cy = 0;
    E.num_rows = 0;
    E.row_offset = 0;
    E.col_offset = 0;

    E.row = NULL;
}


int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    if (argc >= 2) {

        editorOpen(argv[1]);
    }
    while (1) {
        editorRefreshScreen();
        editorReadEvent();
    }
    return 0;
}
*/

void enable_raw_mode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disable_raw_mode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);

    term.c_lflag |= (ICANON | ECHO); // Re-enable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

int main() {
    int c;

    // Enable raw mode
    enable_raw_mode();

    printf("Press any key to see its keycode (Press Ctrl+C to exit)\n");

    while (1) {
        char buf[32];
        int index = 0;
        while (index < sizeof(buf) && read(STDIN_FILENO, &buf[index], 1) != -1) {
            printf("%d", buf[index]);
            index++;
        }

        printf("\n");


        if (c == 3) { // Exit on Ctrl+C
            break;
        }
    }

    // Restore terminal settings
    disable_raw_mode();

    return 0;
}
