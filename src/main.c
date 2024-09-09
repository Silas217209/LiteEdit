#include <locale.h>
#include <stdio.h>
#include <string.h>
#include "terminal.h"
#include "tree_sitter/api.h"

#define clamp(x, min, max) (x)<(min) ? (min) : (x)>(max) ? (max) : (x)
#define ctrl(k) ((k) & 0x1f)
#define STX_COLOR(x, y)                                                        \
    (Style) { .fg = (x), .bg = ST_INHERIT, .attr = (y) }

#define CL_BASE 0x1a1b26
#define CL_TEXT 0xc0caf5

#define CL_SURFACE 0x1f1d2e
#define CL_OVERLAY 0x26233a
#define CL_MUTED 0x6e6a86
#define CL_SUBLTE 0x908caa

#define ST_NORMAL STX_COLOR(0xc0caf5, 0)
#define ST_FUNCTION STX_COLOR(0x7aa2f7, 0)
#define ST_FUNCTION_BUILTIN STX_COLOR(0x2ac3de, 0)
#define ST_TYPE STX_COLOR(0x2ac3de, 0)
#define ST_TYPE_BUILTIN STX_COLOR(0x27a1b9, 0)
#define ST_KEYWORD STX_COLOR(0x9d7cd8, 0)
#define ST_KEYWORD_CONTROL STX_COLOR(0xbb9af7, 0)
#define ST_VARIABLE STX_COLOR(0xc0caf5, 0)
#define ST_VARIABLE_PARAMETER STX_COLOR(0xe0af68, 0)
#define ST_CONSTANT STX_COLOR(0xff9e64, 0)
#define ST_CONSTANT_BUILTIN STX_COLOR(0x2ac3de, 0)
#define ST_STRING STX_COLOR(0x9ece6a, 0)
#define ST_COMMENT STX_COLOR(0x565f89, ITALIC)
#define ST_NUMBER STX_COLOR(0xff9e64, 0)
#define ST_OPERATOR STX_COLOR(0x89ddff, 0)
#define ST_PUNCTUATION STX_COLOR(0xc0caf5, 0)
#define ST_LABEL STX_COLOR(0x7aa2f7, 0)

const TSLanguage *tree_sitter_c(void);

enum color { FG = 1, BG };

typedef enum {
    HORIZONTAL,
    VERTICAL,
} direction;

char *tree_sitter_options[] = {
        "function", "function.builtin", "type",        "type.builtin",
        "keyword",  "keyword.control",  "variable",    "variable.parameter",
        "constant", "constant.builtin", "string",      "comment",
        "number",   "operator",         "punctuation", "label",
        NULL, // Null-terminated array
};


typedef enum {
    HL_NORMAL = 0,
    HL_FUNCTION,
    HL_FUNCTION_BUILTIN,
    HL_TYPE,
    HL_TYPE_BUILTIN,
    HL_KEYWORD,
    HL_KEYWORD_CONTROL,
    HL_VARIABLE,
    HL_VARIABLE_PARAMETER,
    HL_CONSTANT,
    HL_CONSTANT_BUILTIN,
    HL_STRING,
    HL_COMMENT,
    HL_NUMBER,
    HL_OPERATOR,
    HL_PUNCTUATION,
    HL_LABEL,
} HighlightType;

typedef struct {
    size_t size;
    char *chars;
} erow;


struct editorConfig {
    int cx, cy;
    int x_start_offset;
    int x_end_offset;
    int y_start_offset;
    int y_end_offset;
    int row_offset;
    int col_offset;
    int screen_cols;
    int screen_rows;
    int num_rows;
    char *text;
    int len_text;
    erow *row;
    TSParser *parser;
    TSTree *tree;
    TSQuery *highlight_query;
    const TSLanguage *language;
    bool needs_redraw;
};

struct editorConfig E;

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    ts_tree_delete(E.tree);

    terminal_end();
    perror("perror msg");
}

void enableRawMode() {
    setlocale(LC_ALL, "");
    if (terminal_init() != 0) {
        die("terminal_init");
    }
    atexit(disableRawMode);
}


void editorDrawLines() {
    for (int y = 0; y < E.screen_rows - E.y_start_offset - E.y_end_offset;
         y++) {
        int filerow = y + E.row_offset;
        if (filerow < E.num_rows) {
            char ln[5];
            int len_ln = 0;
            len_ln += sprintf(ln, "%d", filerow);

            size_t len = E.row[y + E.row_offset].size - E.col_offset;
            if (len < 0)
                len = 0;
            if (len > E.screen_cols - E.x_start_offset - E.x_end_offset)
                len = E.screen_cols - E.x_start_offset - E.x_end_offset;

            for (int x = 0;
                 x < E.screen_cols - E.x_start_offset - E.x_end_offset; x++) {
                int rx = x + E.col_offset;
                terminal_cell_set(
                        x + E.x_start_offset, y + E.y_start_offset,
                        (struct Cell){
                                .ch = x < len ? E.row[y + E.row_offset]
                                                        .chars[rx]
                                              : ' ',
                                ST_NORMAL,
                        });
            }

            continue;
        }
        terminal_cell_set(0, y + E.y_start_offset,
                          (struct Cell){
                                  .ch = '~',
                                  (Style){
                                          .fg = CL_TEXT,
                                          .bg = CL_BASE,
                                          .attr = 0,
                                  },
                          });
    }
}

void debug() {
    char buf[80];
    int len = 0;

    len += sprintf(buf, "E.cx: %d; E.cy: %d, coloff: %d, rowoff: %d        ",
                   E.cx, E.cy, E.col_offset, E.row_offset);
    for (int i = 0; i < len; i++) {
        terminal_cell_set(i, E.screen_rows,
                          (struct Cell){
                                  .ch = buf[i],
                                  .s = ST_NORMAL,
                          });
    }
}

TSQuery *load_highlight_query() {
    FILE *query_file = fopen(
            "/home/silas/.config/LiteEdit/tree-sitter/c/highlights.scm", "r");
    if (!query_file) {
        perror("Failed to open highlights.scm");
        die("fopen");
    }

    fseek(query_file, 0, SEEK_END);
    long query_size = ftell(query_file);
    fseek(query_file, 0, SEEK_SET);

    char *query_string = malloc(query_size + 1);
    if (!query_string) {
        fclose(query_file);
        die("malloc");
    }

    fread(query_string, 1, query_size, query_file);
    query_string[query_size] = '\0';

    fclose(query_file);

    uint32_t error_offset;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(E.language, query_string, query_size,
                                  &error_offset, &error_type);

    free(query_string);
    if (!query) {
        fprintf(stderr,
                "Failed to create query: error at offset %u, error type %d\n",
                error_offset, error_type);
        return NULL;
    }

    return query;
}

HighlightType get_highlight_type(const char *capture_name, uint32_t len) {
    int match_len = 0;
    int match_index = -1;

    int i = 0;
    while (tree_sitter_options[i] != NULL) {
        const char *str = tree_sitter_options[i];
        int this_match_len = 1;

        int index = 0;
        while (1) {
            if (str[index] == '\0' || index >= len) {
                break;
            }
            if (str[index] != capture_name[index]) {
                this_match_len--;
                break;
            }
            if (str[index] == '.') {
                this_match_len++;
            }

            index++;
        }

        if (this_match_len > match_len) {
            match_len = this_match_len;
            match_index = i;
        }
        i++;
    }

    if (match_index >= 0)
        return (HighlightType) match_index + 1;


    return HL_NORMAL;
}

Style get_style(HighlightType type) {
    switch (type) {
        case HL_NORMAL:
            return ST_NORMAL;
        case HL_FUNCTION:
            return ST_FUNCTION;
        case HL_FUNCTION_BUILTIN:
            return ST_FUNCTION_BUILTIN;
        case HL_TYPE:
            return ST_TYPE;
        case HL_TYPE_BUILTIN:
            return ST_TYPE_BUILTIN;
        case HL_KEYWORD:
            return ST_KEYWORD;
        case HL_KEYWORD_CONTROL:
            return ST_KEYWORD_CONTROL;
        case HL_VARIABLE:
            return ST_VARIABLE;
        case HL_VARIABLE_PARAMETER:
            return ST_VARIABLE_PARAMETER;
        case HL_CONSTANT:
            return ST_CONSTANT;
        case HL_CONSTANT_BUILTIN:
            return ST_CONSTANT_BUILTIN;
        case HL_STRING:
            return ST_STRING;
        case HL_COMMENT:
            return ST_COMMENT;
        case HL_NUMBER:
            return ST_NUMBER;
        case HL_OPERATOR:
            return ST_OPERATOR;
        case HL_PUNCTUATION:
            return ST_PUNCTUATION;
        case HL_LABEL:
            return ST_LABEL;
        default:
            return ST_NORMAL;
    }
}

void apply_highlight(TSNode node, HighlightType hl_type) {
    TSPoint start_point = ts_node_start_point(node);
    TSPoint end_point = ts_node_end_point(node);

    // Adjust start and end rows for vertical scrolling
    int32_t start_row = start_point.row - E.row_offset;
    int32_t end_row = end_point.row - E.row_offset;

    // Only process visible rows
    start_row = (start_row < 0) ? 0 : start_row;
    end_row = (end_row >= E.screen_rows) ? E.screen_rows - 1 : end_row;

    for (int32_t screen_row = start_row; screen_row <= end_row; screen_row++) {
        int32_t file_row = screen_row + E.row_offset;
        if (file_row >= E.num_rows)
            break;

        // Adjust start and end columns for horizontal scrolling
        uint32_t col_start = (file_row == start_point.row)
                                     ? start_point.column - E.col_offset
                                     : 0;
        uint32_t col_end = (file_row == end_point.row)
                                   ? end_point.column - E.col_offset
                                   : E.row[file_row].size - E.col_offset;

        // Ensure columns are within screen bounds
        col_start = (col_start < 0) ? 0 : col_start;
        col_end = (col_end > E.screen_cols) ? E.screen_cols : col_end;

        for (uint32_t screen_col = col_start; screen_col < col_end;
             screen_col++) {
            uint32_t file_col = screen_col + E.col_offset;
            if (file_col >= E.row[file_row].size)
                break;

            char current_char = E.row[file_row].chars[file_col];

            terminal_cell_set(screen_col, screen_row,
                              (struct Cell){
                                      .ch = current_char,
                                      .s = get_style(hl_type),
                              });
        }
    }
}

void editorHighlightSyntax() {
    TSQuery *query = load_highlight_query();
    TSQueryCursor *query_cursor = ts_query_cursor_new();

    TSNode root_node = ts_tree_root_node(E.tree);

    ts_query_cursor_exec(query_cursor, query, root_node);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(query_cursor, &match)) {
        for (uint16_t i = 0; i < match.capture_count; i++) {
            TSQueryCapture capture = match.captures[i];
            uint32_t len;
            const char *capture_name = ts_query_capture_name_for_id(
                    E.highlight_query, capture.index, &len);
            HighlightType hl_type = get_highlight_type(capture_name, len);
            apply_highlight(capture.node, hl_type);
        }
    }

    ts_query_delete(query);
    ts_query_cursor_delete(query_cursor);
}


void editorMoveCursor(direction dir, int value) {
    switch (dir) {
        case HORIZONTAL:
            E.cx += value;
            break;
        case VERTICAL:
            E.cy += value;
            break;
    }
}

void editorScroll() {
    if (E.cy < 1) {
        E.cy = 1;
    }
    if (E.cy > E.num_rows) {
        E.cy = E.num_rows;
    }
    if (E.cy - E.row_offset > E.screen_rows) {
        E.row_offset += E.cy - E.row_offset - E.screen_rows;
    }
    if (E.cy - E.row_offset < 1) {
        E.row_offset += E.cy - E.row_offset - 1;
    }
}

void editorRefreshScreen() {
    editorScroll();
    terminal_move_cursor(E.cx - E.col_offset, E.cy - E.row_offset);
    debug();
    editorDrawLines();
    // editorHighlightSyntax();
    terminal_refresh();
    E.needs_redraw = false;
}

void editorReadEvent() {
    int c = terminal_read_input();

    switch (c) {
        case 'q':
            exit(0);
        case 'h':
            editorMoveCursor(HORIZONTAL, -1);
            break;
        case 'j':
            editorMoveCursor(VERTICAL, 1);
            break;
        case 'k':
            editorMoveCursor(VERTICAL, -1);
            break;
        case 'l':
            editorMoveCursor(HORIZONTAL, 1);
            break;
        case ctrl('u'):
            editorMoveCursor(VERTICAL, -34);
            break;
        case ctrl('d'):
            editorMoveCursor(VERTICAL, 34);
            break;
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

    // load file into E.text
    long length;
    fseek(fp, 0L, SEEK_END);
    length = ftell(fp);
    rewind(fp);

    E.text = (char *) malloc((length + 1) + sizeof(char));
    if (!E.text) {
        die("malloc");
    }

    fread(E.text, sizeof(char), length, fp);
    E.len_text = length;

    fp = fopen(filename, "r");
    if (!fp)
        die("fopen");
    ssize_t linelen;
    size_t len = 0;
    char *line = NULL;

    while ((linelen = getline(&line, &len, fp)) != -1) {
        while (linelen > 0 &&
               (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editorAppenRow(line, linelen);
    }

    free(line);
    fclose(fp);
}


void init_tree_sitter() {
    E.parser = ts_parser_new();

    E.language = tree_sitter_c();

    if (!ts_parser_set_language(E.parser, E.language))
        die("ts_parser_set_language");

    E.highlight_query = load_highlight_query();
}

void update_syntax_tree() {
    E.tree = ts_parser_parse_string(E.parser, E.tree, E.text, E.len_text);
}


void initEditor() {
    terminal_get_size(&E.screen_cols, &E.screen_rows);
    E.cx = 1;
    E.cy = 1;

    E.x_start_offset = 5;
    E.x_end_offset = 5;
    E.y_start_offset = 5;
    E.y_end_offset = 5;

    E.num_rows = 0;
    E.row_offset = 0;
    E.col_offset = 0;
    E.needs_redraw = true;

    E.screen_rows -= 1;

    E.row = NULL;
}


int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    if (argc >= 2) {
        init_tree_sitter();
        editorOpen(argv[1]);
        update_syntax_tree();
    }


    while (1) {
        editorRefreshScreen();
        editorReadEvent();
    }
    return 0;
}
