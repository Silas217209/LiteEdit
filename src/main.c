#define TB_IMPL
#define TB_OPT_ATTR_W 64

#include "termbox2.h"

#include <locale.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tree_sitter/api.h>
#include <unistd.h>
#include <wchar.h>

#define clamp(x, min, max) (x)<(min) ? (min) : (x)>(max) ? (max) : (x)

#define ctrl(k) ((k) & 0x1f)


#define CL_BG 0x1a1b26
#define CL_BG_DARK 0x16161e
#define CL_BG_HIGHLIGHT 0x292e42
#define CL_BLUE 0x7aa2f7
#define CL_BLUE0 0x3d59a1
#define CL_BLUE1 0x2ac3de
#define CL_BLUE2 0x0db9d7
#define CL_BLUE5 0x89ddff
#define CL_BLUE6 0xb4f9f8
#define CL_BLUE7 0x394b70
#define CL_COMMENT 0x565f89
#define CL_CYAN 0x7dcfff
#define CL_DARK3 0x545c7e
#define CL_DARK5 0x737aa2
#define CL_FG 0xC0CAF5
#define CL_FG_DARK 0xA9B1D6
#define CL_FG_GUTTER 0x3b4261
#define CL_GREEN 0x9ece6a
#define CL_GREEN1 0x73daca
#define CL_GREEN2 0x41a6b5
#define CL_MAGENTA 0xBB9AF7
#define CL_MAGENTA2 0xFF007C
#define CL_ORANGE 0xFF9E64
#define CL_PURPLE 0x9D7CD8
#define CL_RED 0xF7768E
#define CL_RED1 0xDB4B4B
#define CL_TEAL 0x1ABC9C
#define CL_TERMINAL_BLACK 0x414868
#define CL_YELLOW 0xE0AF68

#define CL_GIT_ADD 0x449DAB
#define CL_GIT_CHANGE 0x6183BB
#define CL_GIT_DELETE 0x914C54

#define PRE_PROC CL_CYAN
#define BOOLEAN 0xff9e64
#define CHARACTER CL_GREEN
#define SPECIAL 0xff9e64
#define SPECIAL_CHARACTER SPECIAL
#define CONSTANT 0xff9e64
#define DEFINE PRE_PROC
#define CONDITIONAL STATEMENT
#define STATEMENT 0xbb9af7
#define TYPE 0x2ac3de

const TSLanguage *tree_sitter_c(void);

enum color { FG = 1, BG };

typedef enum {
    HORIZONTAL,
    VERTICAL,
} direction;

char *tree_sitter_options[] = {
        "annotation",
        "attribute",
        "boolean",
        "character",
        "character.printf",
        "character.special",
        "comment",
        "comment.error",
        "comment.hint",
        "comment.info",
        "comment.note",
        "comment.todo",
        "comment.warning",
        "constant",
        "constant.builtin",
        "constant.macro",
        "constructor",
        "constructor.tsx",
        "diff.delta",
        "diff.minus",
        "diff.plus",
        "function",
        "function.builtin",
        "function.call",
        "function.macro",
        "function.method",
        "function.method.call",
        "keyword",
        "keyword.conditional",
        "keyword.coroutine",
        "keyword.debug",
        "keyword.directive",
        "keyword.directive.define",
        "keyword.exception",
        "keyword.function",
        "keyword.import",
        "keyword.operator",
        "keyword.repeat",
        "keyword.return",
        "keyword.storage",
        "label",
        "markup",
        "markup.emphasis",
        "markup.environment",
        "markup.environment.name",
        "markup.heading",
        "markup.italic",
        "markup.link",
        "markup.link.label",
        "markup.link.label.symbol",
        "markup.link.url",
        "markup.list",
        "markup.list.checked",
        "markup.list.markdown",
        "markup.list.unchecked",
        "markup.math",
        "markup.raw",
        "markup.raw.markdown_inline",
        "markup.strikethrough",
        "markup.strong",
        "markup.underline",
        "module",
        "module.builtin",
        "namespace.builtin",
        "none",
        "number",
        "number.float",
        "operator",
        "property",
        "punctuation.bracket",
        "punctuation.delimiter",
        "punctuation.special",
        "string",
        "string.documentation",
        "string.escape",
        "string.regexp",
        "tag",
        "tag.attribute",
        "tag.delimiter",
        "tag.delimiter.tsx",
        "tag.tsx",
        "tag.javascript",
        "type",
        "type.builtin",
        "type.definition",
        "type.qualifier",
        "variable",
        "variable.builtin",
        "variable.member",
        "variable.parameter",
        "variable.parameter.builtin",
        NULL,
};


typedef enum {
    HL_NORMAL = 0,
    HL_ANNOTATION,
    HL_ATTRIBUTE,
    HL_BOOLEAN,
    HL_CHARACTER,
    HL_CHARACTER_PRINTF,
    HL_CHARACTER_SPECIAL,
    HL_COMMENT,
    HL_COMMENT_ERROR,
    HL_COMMENT_HINT,
    HL_COMMENT_INFO,
    HL_COMMENT_NOTE,
    HL_COMMENT_TODO,
    HL_COMMENT_WARNING,
    HL_CONSTANT,
    HL_CONSTANT_BUILTIN,
    HL_CONSTANT_MACRO,
    HL_CONSTRUCTOR,
    HL_CONSTRUCTOR_TSX,
    HL_DIFF_DELTA,
    HL_DIFF_MINUS,
    HL_DIFF_PLUS,
    HL_FUNCTION,
    HL_FUNCTION_BUILTIN,
    HL_FUNCTION_CALL,
    HL_FUNCTION_MACRO,
    HL_FUNCTION_METHOD,
    HL_FUNCTION_METHOD_CALL,
    HL_KEYWORD,
    HL_KEYWORD_CONDITIONAL,
    HL_KEYWORD_COROUTINE,
    HL_KEYWORD_DEBUG,
    HL_KEYWORD_DIRECTIVE,
    HL_KEYWORD_DIRECTIVE_DEFINE,
    HL_KEYWORD_EXCEPTION,
    HL_KEYWORD_FUNCTION,
    HL_KEYWORD_IMPORT,
    HL_KEYWORD_OPERATOR,
    HL_KEYWORD_REPEAT,
    HL_KEYWORD_RETURN,
    HL_KEYWORD_STORAGE,
    HL_LABEL,
    HL_MARKUP,
    HL_MARKUP_EMPHASIS,
    HL_MARKUP_ENVIRONMENT,
    HL_MARKUP_ENVIRONMENT_NAME,
    HL_MARKUP_HEADING,
    HL_MARKUP_ITALIC,
    HL_MARKUP_LINK,
    HL_MARKUP_LINK_LABEL,
    HL_MARKUP_LINK_LABEL_SYMBOL,
    HL_MARKUP_LINK_URL,
    HL_MARKUP_LIST,
    HL_MARKUP_LIST_CHECKED,
    HL_MARKUP_LIST_MARKDOWN,
    HL_MARKUP_LIST_UNCHECKED,
    HL_MARKUP_MATH,
    HL_MARKUP_RAW,
    HL_MARKUP_RAW_MARKDOWN_INLINE,
    HL_MARKUP_STRIKETHROUGH,
    HL_MARKUP_STRONG,
    HL_MARKUP_UNDERLINE,
    HL_MODULE,
    HL_MODULE_BUILTIN,
    HL_NAMESPACE_BUILTIN,
    HL_NONE,
    HL_NUMBER,
    HL_NUMBER_FLOAT,
    HL_OPERATOR,
    HL_PROPERTY,
    HL_PUNCTUATION_BRACKET,
    HL_PUNCTUATION_DELIMITER,
    HL_PUNCTUATION_SPECIAL,
    HL_STRING,
    HL_STRING_DOCUMENTATION,
    HL_STRING_ESCAPE,
    HL_STRING_REGEXP,
    HL_TAG,
    HL_TAG_ATTRIBUTE,
    HL_TAG_DELIMITER,
    HL_TAG_DELIMITER_TSX,
    HL_TAG_TSX,
    HL_TAG_JAVASCRIPT,
    HL_TYPE,
    HL_TYPE_BUILTIN,
    HL_TYPE_DEFINITION,
    HL_TYPE_QUALIFIER,
    HL_VARIABLE,
    HL_VARIABLE_BUILTIN,
    HL_VARIABLE_MEMBER,
    HL_VARIABLE_PARAMETER,
    HL_VARIABLE_PARAMETER_BUILTIN,
} HighlightType;

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
    char *text;
    int len_text;
    erow *row;
    TSParser *parser;
    TSTree *tree;
    TSQuery *highlight_query;
    const TSLanguage *language;
};

struct editorConfig E;

void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    tb_shutdown();
    perror("perror msg");
}

void enableRawMode() {
    setlocale(LC_ALL, "");
    if (tb_init() != 0) {
        die("tb_init");
    }
    atexit(disableRawMode);
    tb_set_output_mode(TB_OUTPUT_TRUECOLOR);
}


void editorDrawLines() {
    for (int y = 0; y < E.screen_rows; y++) {
        if (y + E.row_offset < E.num_rows) {
            size_t len = E.row[y + E.row_offset].size - E.col_offset;
            if (len < 0)
                len = 0;
            if (len > E.screen_cols)
                len = E.screen_cols;

            for (int x = 0; x < E.screen_cols; x++) {
                int rx = x + E.col_offset;
                tb_set_cell(x, y, x < len ? E.row[y + E.row_offset].chars[rx] : ' ', CL_FG, CL_BG);
            }

            continue;
        }
        tb_print(0, y, TB_WHITE, TB_DEFAULT, "~");
    }
}

TSQuery *load_highlight_query() {
    FILE *query_file = fopen("/home/silas/.config/LiteEdit/tree-sitter/c/highlights.scm", "r");
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
    TSQuery *query = ts_query_new(E.language, query_string, query_size, &error_offset, &error_type);

    free(query_string);
    if (!query) {
        fprintf(stderr, "Failed to create query: error at offset %u, error type %d\n", error_offset, error_type);
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

uint64_t get_color(HighlightType type) {
    switch (type) {
        case HL_NORMAL:
            return CL_FG;
        case HL_ANNOTATION:
            return PRE_PROC;
        case HL_ATTRIBUTE:
            return PRE_PROC;
        case HL_BOOLEAN:
            return BOOLEAN;
        case HL_CHARACTER:
            return CHARACTER;
        case HL_CHARACTER_PRINTF:
            return SPECIAL_CHARACTER;
        case HL_CHARACTER_SPECIAL:
            return SPECIAL_CHARACTER;
        case HL_COMMENT:
            return CL_COMMENT;
        case HL_COMMENT_ERROR:
            return CL_RED1;
        case HL_COMMENT_HINT:
            return CL_TEAL;
        case HL_COMMENT_INFO:
            return CL_BLUE2;
        case HL_COMMENT_NOTE:
            return CL_TEAL;
        case HL_COMMENT_TODO:
            return CL_BLUE;
        case HL_COMMENT_WARNING:
            return CL_YELLOW;
        case HL_CONSTANT:
            return CONSTANT;
        case HL_CONSTANT_BUILTIN:
            return SPECIAL;
        case HL_CONSTANT_MACRO:
            return DEFINE;
        case HL_CONSTRUCTOR:
            return CL_MAGENTA;
        case HL_CONSTRUCTOR_TSX:
            return CL_BLUE1;
        case HL_DIFF_DELTA:
            return CL_BLUE7;
        case HL_DIFF_MINUS:
            return CL_RED1;
        case HL_DIFF_PLUS:
            return CL_GREEN2;
        case HL_FUNCTION:
            return CL_BLUE;
        case HL_FUNCTION_BUILTIN:
            return SPECIAL;
        case HL_FUNCTION_CALL:
            return CL_BLUE;
        case HL_FUNCTION_MACRO:
            return PRE_PROC;
        case HL_FUNCTION_METHOD:
            return CL_BLUE;
        case HL_FUNCTION_METHOD_CALL:
            return CL_BLUE;
        case HL_KEYWORD:
            return CL_PURPLE;
        case HL_KEYWORD_CONDITIONAL:
            return CONDITIONAL;
        case HL_KEYWORD_COROUTINE:
            return CL_PURPLE;
        case HL_KEYWORD_DEBUG:
            return CL_ORANGE;
        case HL_KEYWORD_DIRECTIVE:
            return PRE_PROC;
        case HL_KEYWORD_DIRECTIVE_DEFINE:
            return DEFINE;
        case HL_KEYWORD_EXCEPTION:
            return STATEMENT;
        case HL_KEYWORD_FUNCTION:
            return CL_MAGENTA;
        case HL_KEYWORD_IMPORT:
            return PRE_PROC;
        case HL_KEYWORD_OPERATOR:
            return CL_BLUE5;
        case HL_KEYWORD_REPEAT:
            return STATEMENT;
        case HL_KEYWORD_RETURN:
            return CL_PURPLE;
        case HL_KEYWORD_STORAGE:
            return TYPE;
        case HL_LABEL:
            return CL_BLUE;
        case HL_MARKUP:
        case HL_MARKUP_EMPHASIS:
        case HL_MARKUP_ENVIRONMENT:
        case HL_MARKUP_ENVIRONMENT_NAME:
        case HL_MARKUP_HEADING:
        case HL_MARKUP_ITALIC:
        case HL_MARKUP_LINK:
        case HL_MARKUP_LINK_LABEL:
        case HL_MARKUP_LINK_LABEL_SYMBOL:
        case HL_MARKUP_LINK_URL:
        case HL_MARKUP_LIST:
        case HL_MARKUP_LIST_CHECKED:
        case HL_MARKUP_LIST_MARKDOWN:
        case HL_MARKUP_LIST_UNCHECKED:
        case HL_MARKUP_MATH:
        case HL_MARKUP_RAW:
        case HL_MARKUP_RAW_MARKDOWN_INLINE:
        case HL_MARKUP_STRIKETHROUGH:
        case HL_MARKUP_STRONG:
        case HL_MARKUP_UNDERLINE:
            return CL_FG;
        case HL_MODULE:
            return PRE_PROC;
        case HL_MODULE_BUILTIN:
            return CL_RED;
        case HL_NAMESPACE_BUILTIN:
            return CL_RED;
        case HL_NONE:
            return CL_FG;
        case HL_NUMBER:
            return CONSTANT;
        case HL_NUMBER_FLOAT:
            return CONSTANT;
        case HL_OPERATOR:
            return CL_BLUE5;
        case HL_PROPERTY:
            return CL_GREEN1;
        case HL_PUNCTUATION_BRACKET:
            return CL_FG_DARK;
        case HL_PUNCTUATION_DELIMITER:
            return CL_BLUE5;
        case HL_PUNCTUATION_SPECIAL:
            return CL_BLUE5;
        case HL_STRING:
            return CL_GREEN;
        case HL_STRING_DOCUMENTATION:
            return CL_YELLOW;
        case HL_STRING_ESCAPE:
            return CL_MAGENTA;
        case HL_STRING_REGEXP:
            return CL_BLUE6;
        case HL_TAG:
            return STATEMENT;
        case HL_TAG_ATTRIBUTE:
            return CL_GREEN1;
        case HL_TAG_DELIMITER:
            return SPECIAL;
        case HL_TAG_DELIMITER_TSX:
            return CL_BLUE;
        case HL_TAG_TSX:
            return CL_RED;
        case HL_TAG_JAVASCRIPT:
            return CL_RED;
        case HL_TYPE:
            return TYPE;
        case HL_TYPE_BUILTIN:
            return CL_RED;
        case HL_TYPE_DEFINITION:
            return TYPE;
        case HL_TYPE_QUALIFIER:
            return CL_PURPLE;
        case HL_VARIABLE:
            return CL_FG;
        case HL_VARIABLE_BUILTIN:
            return CL_RED;
        case HL_VARIABLE_MEMBER:
            return CL_GREEN1;
        case HL_VARIABLE_PARAMETER:
            return CL_YELLOW;
        case HL_VARIABLE_PARAMETER_BUILTIN:
            return CL_YELLOW;
        default:
            return CL_FG;
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
        uint32_t col_start = (file_row == start_point.row) ? start_point.column - E.col_offset : 0;
        uint32_t col_end =
                (file_row == end_point.row) ? end_point.column - E.col_offset : E.row[file_row].size - E.col_offset;

        // Ensure columns are within screen bounds
        col_start = (col_start < 0) ? 0 : col_start;
        col_end = (col_end > E.screen_cols) ? E.screen_cols : col_end;

        for (uint32_t screen_col = col_start; screen_col < col_end; screen_col++) {
            uint32_t file_col = screen_col + E.col_offset;
            if (file_col >= E.row[file_row].size)
                break;

            char current_char = E.row[file_row].chars[file_col];
            tb_set_cell(screen_col, screen_row, current_char, get_color(hl_type), CL_BG);
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
            const char *capture_name = ts_query_capture_name_for_id(E.highlight_query, capture.index, &len);
            HighlightType hl_type = get_highlight_type(capture_name, len);
            apply_highlight(capture.node, hl_type);
        }
    }

    ts_query_delete(query);
    ts_query_cursor_delete(query_cursor);
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
            E.cy = clamp(E.cy, 0, E.num_rows - 1);
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
    // editorDrawLines();
    editorHighlightSyntax();
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
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
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

void update_syntax_tree() { E.tree = ts_parser_parse_string(E.parser, E.tree, E.text, E.len_text); }


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
