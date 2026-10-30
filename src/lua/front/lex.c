
#include "lex.h"
#include "../../ir/scratch.h"
#include "../../value/error.h"
#include "../../value/obj.h"
#include "../../value/string.h"

const char *const mx_lex_type_names[] = {
    [MX_LEX_TYPE_EOF] = "<eof>",
    [MX_LEX_TYPE_NAME] = "<name>",
    [MX_LEX_TYPE_NUMBER] = "<number>",
    [MX_LEX_TYPE_STRING] = "<string>",
    [MX_LEX_TYPE_AND] = "and",
    [MX_LEX_TYPE_BREAK] = "break",
    [MX_LEX_TYPE_DO] = "do",
    [MX_LEX_TYPE_ELSE] = "else",
    [MX_LEX_TYPE_ELSEIF] = "elseif",
    [MX_LEX_TYPE_END] = "end",
    [MX_LEX_TYPE_FALSE] = "false",
    [MX_LEX_TYPE_FOR] = "for",
    [MX_LEX_TYPE_FUNCTION] = "function",
    [MX_LEX_TYPE_GOTO] = "goto",
    [MX_LEX_TYPE_IF] = "if",
    [MX_LEX_TYPE_IN] = "in",
    [MX_LEX_TYPE_LOCAL] = "local",
    [MX_LEX_TYPE_NIL] = "nil",
    [MX_LEX_TYPE_NOT] = "not",
    [MX_LEX_TYPE_OR] = "or",
    [MX_LEX_TYPE_REPEAT] = "repeat",
    [MX_LEX_TYPE_RETURN] = "return",
    [MX_LEX_TYPE_THEN] = "then",
    [MX_LEX_TYPE_TRUE] = "true",
    [MX_LEX_TYPE_UNTIL] = "until",
    [MX_LEX_TYPE_WHILE] = "while",
    [MX_LEX_TYPE_PLUS] = "+",
    [MX_LEX_TYPE_MINUS] = "-",
    [MX_LEX_TYPE_STAR] = "*",
    [MX_LEX_TYPE_SLASH] = "/",
    [MX_LEX_TYPE_SLASH2] = "//",
    [MX_LEX_TYPE_PERCENT] = "%",
    [MX_LEX_TYPE_CARET] = "^",
    [MX_LEX_TYPE_HASH] = "#",
    [MX_LEX_TYPE_AMP] = "&",
    [MX_LEX_TYPE_TILDE] = "~",
    [MX_LEX_TYPE_PIPE] = "|",
    [MX_LEX_TYPE_SHL] = "<<",
    [MX_LEX_TYPE_SHR] = ">>",
    [MX_LEX_TYPE_EQ2] = "==",
    [MX_LEX_TYPE_NE] = "~=",
    [MX_LEX_TYPE_LE] = "<=",
    [MX_LEX_TYPE_GE] = ">=",
    [MX_LEX_TYPE_LT] = "<",
    [MX_LEX_TYPE_GT] = ">",
    [MX_LEX_TYPE_ASSIGN] = "=",
    [MX_LEX_TYPE_LPAREN] = "(",
    [MX_LEX_TYPE_RPAREN] = ")",
    [MX_LEX_TYPE_LBRACE] = "{",
    [MX_LEX_TYPE_RBRACE] = "}",
    [MX_LEX_TYPE_LBRACKET] = "[",
    [MX_LEX_TYPE_RBRACKET] = "]",
    [MX_LEX_TYPE_SEMI] = ";",
    [MX_LEX_TYPE_COLON] = ":",
    [MX_LEX_TYPE_LABEL] = "::",
    [MX_LEX_TYPE_COMMA] = ",",
    [MX_LEX_TYPE_DOT] = ".",
    [MX_LEX_TYPE_CONCAT] = "..",
    [MX_LEX_TYPE_DOTS] = "...",
};

const char *mx_lex_type_name(mx_lex_type_t type) {
    return mx_lex_type_names[type];
}

mx_obj_t mx_lex_tok_value(const mx_lex_token_t *tok) {
    return mx_obj_of_raw(tok->kind, tok->val);
}

mx_lex_scan_t mx_lex_scan_value(mx_lex_type_t type, uint32_t start, uint32_t stop, mx_obj_t v) {
    return (mx_lex_scan_t) {
        .tok = (mx_lex_token_t) {
            .start = start,
            .stop = stop,
            .val = v.val,
            .kind = v.kind,
            .type = type,
        },
        .error = NULL,
    };
}

const mx_lex_class_t mx_lex_classes[256] = {
    ['\t'] = MX_LEX_CLASS_SPACE, ['\n'] = MX_LEX_CLASS_SPACE, ['\v'] = MX_LEX_CLASS_SPACE,
    ['\f'] = MX_LEX_CLASS_SPACE, ['\r'] = MX_LEX_CLASS_SPACE, [' '] = MX_LEX_CLASS_SPACE,
    ['0'] = MX_LEX_CLASS_DIGIT, ['1'] = MX_LEX_CLASS_DIGIT, ['2'] = MX_LEX_CLASS_DIGIT,
    ['3'] = MX_LEX_CLASS_DIGIT, ['4'] = MX_LEX_CLASS_DIGIT, ['5'] = MX_LEX_CLASS_DIGIT,
    ['6'] = MX_LEX_CLASS_DIGIT, ['7'] = MX_LEX_CLASS_DIGIT, ['8'] = MX_LEX_CLASS_DIGIT,
    ['9'] = MX_LEX_CLASS_DIGIT, ['_'] = MX_LEX_CLASS_NAME_START,
    ['a'] = MX_LEX_CLASS_NAME_START, ['b'] = MX_LEX_CLASS_NAME_START, ['c'] = MX_LEX_CLASS_NAME_START,
    ['d'] = MX_LEX_CLASS_NAME_START, ['e'] = MX_LEX_CLASS_NAME_START, ['f'] = MX_LEX_CLASS_NAME_START,
    ['g'] = MX_LEX_CLASS_NAME_START, ['h'] = MX_LEX_CLASS_NAME_START, ['i'] = MX_LEX_CLASS_NAME_START,
    ['j'] = MX_LEX_CLASS_NAME_START, ['k'] = MX_LEX_CLASS_NAME_START, ['l'] = MX_LEX_CLASS_NAME_START,
    ['m'] = MX_LEX_CLASS_NAME_START, ['n'] = MX_LEX_CLASS_NAME_START, ['o'] = MX_LEX_CLASS_NAME_START,
    ['p'] = MX_LEX_CLASS_NAME_START, ['q'] = MX_LEX_CLASS_NAME_START, ['r'] = MX_LEX_CLASS_NAME_START,
    ['s'] = MX_LEX_CLASS_NAME_START, ['t'] = MX_LEX_CLASS_NAME_START, ['u'] = MX_LEX_CLASS_NAME_START,
    ['v'] = MX_LEX_CLASS_NAME_START, ['w'] = MX_LEX_CLASS_NAME_START, ['x'] = MX_LEX_CLASS_NAME_START,
    ['y'] = MX_LEX_CLASS_NAME_START, ['z'] = MX_LEX_CLASS_NAME_START,
    ['A'] = MX_LEX_CLASS_NAME_START, ['B'] = MX_LEX_CLASS_NAME_START, ['C'] = MX_LEX_CLASS_NAME_START,
    ['D'] = MX_LEX_CLASS_NAME_START, ['E'] = MX_LEX_CLASS_NAME_START, ['F'] = MX_LEX_CLASS_NAME_START,
    ['G'] = MX_LEX_CLASS_NAME_START, ['H'] = MX_LEX_CLASS_NAME_START, ['I'] = MX_LEX_CLASS_NAME_START,
    ['J'] = MX_LEX_CLASS_NAME_START, ['K'] = MX_LEX_CLASS_NAME_START, ['L'] = MX_LEX_CLASS_NAME_START,
    ['M'] = MX_LEX_CLASS_NAME_START, ['N'] = MX_LEX_CLASS_NAME_START, ['O'] = MX_LEX_CLASS_NAME_START,
    ['P'] = MX_LEX_CLASS_NAME_START, ['Q'] = MX_LEX_CLASS_NAME_START, ['R'] = MX_LEX_CLASS_NAME_START,
    ['S'] = MX_LEX_CLASS_NAME_START, ['T'] = MX_LEX_CLASS_NAME_START, ['U'] = MX_LEX_CLASS_NAME_START,
    ['V'] = MX_LEX_CLASS_NAME_START, ['W'] = MX_LEX_CLASS_NAME_START, ['X'] = MX_LEX_CLASS_NAME_START,
    ['Y'] = MX_LEX_CLASS_NAME_START, ['Z'] = MX_LEX_CLASS_NAME_START,
};

bool mx_lex_byte_is_name_start(uint8_t c) {
    return (mx_lex_classes[c] & MX_LEX_CLASS_NAME_START) != 0;
}

bool mx_lex_byte_is_name(uint8_t c) {
    return (mx_lex_classes[c] & (MX_LEX_CLASS_NAME_START | MX_LEX_CLASS_DIGIT)) != 0;
}

bool mx_lex_byte_is_digit(uint8_t c) {
    return (mx_lex_classes[c] & MX_LEX_CLASS_DIGIT) != 0;
}

bool mx_lex_byte_is_hex(uint8_t c) {
    return mx_lex_byte_is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool mx_lex_byte_is_space(uint8_t c) {
    return (mx_lex_classes[c] & MX_LEX_CLASS_SPACE) != 0;
}

uint64_t mx_lex_hex_value(uint8_t c) {
    if (mx_lex_byte_is_digit(c)) {
        return (uint64_t)(c - '0');
    } else {
        return (uint64_t)((c | 0x20) - 'a' + 10);
    }
}

mx_error_t *mx_lex_error_finish(mx_lex_t *lex, mx_str_t *msg, uint32_t at) {
    mx_location_range_t range = {
        .source = lex->source,
        .start = at,
    };
    mx_obj_t error = mx_error_message(lex->vm, &range, "%s", msg->buf);
    mx_str_free(msg);
    return mx_obj_get_error(error);
}

mx_error_t *mx_lex_error_slice(mx_lex_t *lex, uint32_t start, uint32_t stop, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new();
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    if (stop > start) {
        mx_str_format(msg, " near '");
        mx_str_append(msg, &lex->src[start], stop - start);
        mx_str_format(msg, "'");
    } else {
        mx_str_format(msg, " near <eof>");
    }
    return mx_lex_error_finish(lex, msg, start);
}

mx_error_t *mx_lex_error_plain(mx_lex_t *lex, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new();
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    return mx_lex_error_finish(lex, msg, lex->tok.start);
}

mx_error_t *mx_lex_error_near(mx_lex_t *lex, const char *fmt, ...) {
    mx_str_t *msg = mx_str_new();
    va_list ap;
    va_start(ap, fmt);
    mx_str_vformat(msg, fmt, ap);
    va_end(ap);
    if (lex->tok.type == MX_LEX_TYPE_EOF) {
        mx_str_format(msg, " near <eof>");
    } else {
        mx_str_format(msg, " near '");
        mx_str_append(msg, &lex->src[lex->tok.start], lex->tok.stop - lex->tok.start);
        mx_str_format(msg, "'");
    }
    return mx_lex_error_finish(lex, msg, lex->tok.start);
}

uint8_t mx_lex_at(mx_lex_t *lex, uint32_t pos) {
    if (pos < lex->len) {
        return (uint8_t)lex->src[pos];
    } else {
        return 0;
    }
}

size_t mx_lex_line_of(mx_lex_t *lex, uint32_t pos) {
    return mx_source_line(lex->source, pos);
}

int64_t mx_lex_open_level(mx_lex_t *lex, uint32_t pos) {
    uint32_t at = pos + 1;
    while (mx_lex_at(lex, at) == '=') {
        at += 1;
    }
    if (mx_lex_at(lex, at) == '[') {
        return (int64_t)(at - pos - 1);
    } else {
        return -1;
    }
}

mx_lex_long_t mx_lex_long_body(mx_lex_t *lex, uint32_t open_start, int64_t level, const char *what) {
    uint8_t first = mx_lex_at(lex, lex->pos);
    if (first == '\r' || first == '\n') {
        lex->pos += 1;
        uint8_t second = mx_lex_at(lex, lex->pos);
        if ((second == '\r' || second == '\n') && second != first) {
            lex->pos += 1;
        }
    }
    uint32_t content_start = lex->pos;
    while (true) {
        if (lex->pos >= lex->len) {
            size_t line = mx_lex_line_of(lex, open_start);
            return (mx_lex_long_t) {
                .error = mx_lex_error_slice(lex, lex->len, lex->len, "unfinished long %s (starting at line %zu)", what, line),
                .content_start = content_start,
                .content_stop = content_start,
            };
        } else if (mx_lex_at(lex, lex->pos) == ']') {
            uint32_t at = lex->pos + 1;
            int64_t close = 0;
            while (mx_lex_at(lex, at) == '=') {
                at += 1;
                close += 1;
            }
            if (close == level && mx_lex_at(lex, at) == ']') {
                uint32_t content_stop = lex->pos;
                lex->pos = at + 1;
                return (mx_lex_long_t) {
                    .error = NULL,
                    .content_start = content_start,
                    .content_stop = content_stop,
                };
            } else {
                lex->pos += 1;
            }
        } else {
            lex->pos += 1;
        }
    }
}

mx_obj_t mx_lex_long_value(mx_lex_t *lex, uint32_t start, uint32_t stop) {
    mx_str_t *buf = lex->scratch;
    buf->len = 0;
    uint32_t at = start;
    while (at < stop) {
        uint8_t c = mx_lex_at(lex, at);
        at += 1;
        if (c == '\n' || c == '\r') {
            mx_str_append_byte(buf, '\n');
            uint8_t second = mx_lex_at(lex, at);
            if (at < stop && (second == '\n' || second == '\r') && second != c) {
                at += 1;
            }
        } else {
            mx_str_append_byte(buf, (char)c);
        }
    }
    return mx_obj_intern_strn(lex->vm, buf->buf, buf->len);
}

void mx_lex_utf8_encode(mx_str_t *buf, uint64_t cp) {
    if (cp < 0x80) {
        mx_str_append_byte(buf, (char)cp);
    } else {
        char tail[6];
        uint32_t n = 0;
        uint64_t room = 0x3f;
        while (cp > room) {
            tail[n] = (char)(0x80 | (cp & 0x3f));
            n += 1;
            cp >>= 6;
            room >>= 1;
        }
        mx_str_append_byte(buf, (char)((~room << 1) | cp));
        while (n > 0) {
            n -= 1;
            mx_str_append_byte(buf, tail[n]);
        }
    }
}

mx_error_t *mx_lex_escape_error(mx_lex_t *lex, uint32_t start, const char *msg) {
    uint32_t stop;
    if (lex->pos < lex->len) {
        stop = lex->pos + 1;
    } else {
        stop = lex->pos;
    }
    return mx_lex_error_slice(lex, start, stop, "%s", msg);
}

mx_error_t *mx_lex_escape(mx_lex_t *lex, mx_str_t *buf, uint32_t start) {
    uint8_t c = mx_lex_at(lex, lex->pos);
    switch (c) {
        case 'n': {
            mx_str_append_byte(buf, '\n');
            lex->pos += 1;
            return NULL;
        }
        case 't': {
            mx_str_append_byte(buf, '\t');
            lex->pos += 1;
            return NULL;
        }
        case 'r': {
            mx_str_append_byte(buf, '\r');
            lex->pos += 1;
            return NULL;
        }
        case 'a': {
            mx_str_append_byte(buf, '\a');
            lex->pos += 1;
            return NULL;
        }
        case 'b': {
            mx_str_append_byte(buf, '\b');
            lex->pos += 1;
            return NULL;
        }
        case 'f': {
            mx_str_append_byte(buf, '\f');
            lex->pos += 1;
            return NULL;
        }
        case 'v': {
            mx_str_append_byte(buf, '\v');
            lex->pos += 1;
            return NULL;
        }
        case '\\':
        case '"':
        case '\'': {
            mx_str_append_byte(buf, (char)c);
            lex->pos += 1;
            return NULL;
        }
        case '\n':
        case '\r': {
            mx_str_append_byte(buf, '\n');
            lex->pos += 1;
            uint8_t second = mx_lex_at(lex, lex->pos);
            if ((second == '\n' || second == '\r') && second != c) {
                lex->pos += 1;
            }
            return NULL;
        }
        case 'x': {
            lex->pos += 1;
            uint64_t val = 0;
            for (size_t i = 0; i < 2; i++) {
                uint8_t h = mx_lex_at(lex, lex->pos);
                if (!mx_lex_byte_is_hex(h)) {
                    return mx_lex_escape_error(lex, start, "hexadecimal digit expected");
                }
                val = val * 16 + mx_lex_hex_value(h);
                lex->pos += 1;
            }
            mx_str_append_byte(buf, (char)val);
            return NULL;
        }
        case 'u': {
            lex->pos += 1;
            if (mx_lex_at(lex, lex->pos) != '{') {
                return mx_lex_escape_error(lex, start, "missing '{' in \\u{xxxx}");
            }
            lex->pos += 1;
            if (!mx_lex_byte_is_hex(mx_lex_at(lex, lex->pos))) {
                return mx_lex_escape_error(lex, start, "hexadecimal digit expected");
            }
            uint64_t cp = 0;
            while (mx_lex_byte_is_hex(mx_lex_at(lex, lex->pos))) {
                cp = cp * 16 + mx_lex_hex_value(mx_lex_at(lex, lex->pos));
                if (cp > 0x7FFFFFFF) {
                    return mx_lex_escape_error(lex, start, "UTF-8 value too large");
                }
                lex->pos += 1;
            }
            if (mx_lex_at(lex, lex->pos) != '}') {
                return mx_lex_escape_error(lex, start, "missing '}' in \\u{xxxx}");
            }
            lex->pos += 1;
            mx_lex_utf8_encode(buf, cp);
            return NULL;
        }
        case 'z': {
            lex->pos += 1;
            while (lex->pos < lex->len && mx_lex_byte_is_space(mx_lex_at(lex, lex->pos))) {
                lex->pos += 1;
            }
            return NULL;
        }
        default: {
            if (!mx_lex_byte_is_digit(c)) {
                return mx_lex_escape_error(lex, start, "invalid escape sequence");
            }
            uint64_t val = 0;
            size_t n = 0;
            while (n < 3 && mx_lex_byte_is_digit(mx_lex_at(lex, lex->pos))) {
                val = val * 10 + (uint64_t)(mx_lex_at(lex, lex->pos) - '0');
                lex->pos += 1;
                n += 1;
            }
            if (val > 255) {
                return mx_lex_escape_error(lex, start, "decimal escape too large");
            }
            mx_str_append_byte(buf, (char)val);
            return NULL;
        }
    }
}

mx_lex_scan_t mx_lex_short_string(mx_lex_t *lex) {
    uint32_t start = lex->pos;
    uint8_t quote = mx_lex_at(lex, lex->pos);
    lex->pos += 1;
    mx_str_t *buf = lex->scratch;
    buf->len = 0;
    while (true) {
        uint8_t c = mx_lex_at(lex, lex->pos);
        if (lex->pos >= lex->len) {
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, lex->len, lex->len, "unfinished string") };
        } else if (c == quote) {
            lex->pos += 1;
            return mx_lex_scan_value(MX_LEX_TYPE_STRING, start, lex->pos, mx_obj_intern_strn(lex->vm, buf->buf, buf->len));
        } else if (c == '\n' || c == '\r') {
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, lex->pos, "unfinished string") };
        } else if (c != '\\') {
            mx_str_append_byte(buf, (char)c);
            lex->pos += 1;
        } else if (lex->pos + 1 >= lex->len) {
            lex->pos += 1;
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, lex->len, lex->len, "unfinished string") };
        } else {
            lex->pos += 1;
            mx_error_t *error = mx_lex_escape(lex, buf, start);
            if (error != NULL) {
                return (mx_lex_scan_t) { .error = error };
            }
        }
    }
}

mx_lex_scan_t mx_lex_number(mx_lex_t *lex) {
    uint32_t start = lex->pos;
    uint8_t x = mx_lex_at(lex, lex->pos + 1);
    bool is_hex = mx_lex_at(lex, lex->pos) == '0' && (x == 'x' || x == 'X');
    const char *expo;
    if (is_hex) {
        expo = "pP";
        lex->pos += 2;
    } else {
        expo = "eE";
    }
    bool is_int = true;
    while (lex->pos < lex->len) {
        uint8_t c = mx_lex_at(lex, lex->pos);
        if (c == (uint8_t)expo[0] || c == (uint8_t)expo[1]) {
            is_int = false;
            lex->pos += 1;
            uint8_t sign = mx_lex_at(lex, lex->pos);
            if (sign == '+' || sign == '-') {
                lex->pos += 1;
            }
        } else if (c == '.') {
            is_int = false;
            lex->pos += 1;
        } else if (mx_lex_byte_is_digit(c) || (is_hex && mx_lex_byte_is_hex(c))) {
            lex->pos += 1;
        } else {
            break;
        }
    }
    if (lex->pos < lex->len && mx_lex_byte_is_name(mx_lex_at(lex, lex->pos))) {
        lex->pos += 1;
        uint32_t bad_stop;
        if (lex->pos < lex->len && mx_lex_byte_is_name(mx_lex_at(lex, lex->pos))) {
            bad_stop = lex->pos + 1;
        } else {
            bad_stop = lex->pos;
        }
        return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, bad_stop, "malformed number") };
    }
    uint32_t stop = lex->pos;
    mx_str_t *buf = lex->scratch;
    buf->len = 0;
    mx_str_append(buf, &lex->src[start], stop - start);
    mx_str_append_byte(buf, '\0');
    const char *text = buf->buf;
    if (is_int && is_hex) {
        if (stop - start <= 2) {
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, stop, "malformed number") };
        }
        uint64_t val = 0;
        for (uint32_t at = start + 2; at < stop; at++) {
            val = val * 16 + mx_lex_hex_value((uint8_t)lex->src[at]);
        }
        return mx_lex_scan_value(MX_LEX_TYPE_NUMBER, start, stop, mx_obj_of_int((int64_t)val));
    } else if (is_int) {
        errno = 0;
        char *end;
        int64_t val = strtoll(text, &end, 10);
        if (errno == ERANGE || *end != '\0') {
            return mx_lex_scan_value(MX_LEX_TYPE_NUMBER, start, stop, mx_obj_of_number(strtod(text, NULL)));
        } else {
            return mx_lex_scan_value(MX_LEX_TYPE_NUMBER, start, stop, mx_obj_of_int(val));
        }
    } else {
        char *first_end;
        double first = strtod(text, &first_end);
        char point = localeconv()->decimal_point[0];
        char *dot = strchr(buf->buf, '.');
        char *end;
        double val;
        if (*first_end != '\0' && point != '.' && dot != NULL) {
            *dot = point;
            val = strtod(text, &end);
        } else {
            val = first;
            end = first_end;
        }
        if (*end != '\0' || end == text) {
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, stop, "malformed number") };
        } else {
            return mx_lex_scan_value(MX_LEX_TYPE_NUMBER, start, stop, mx_obj_of_number(val));
        }
    }
}

mx_lex_type_t mx_lex_keyword(const char *src, uint32_t start, uint32_t stop) {
    uint32_t len = stop - start;
    if (len < 2 || len > 8) {
        return MX_LEX_TYPE_NAME;
    }
    const char *s = &src[start];
    mx_lex_type_t type;
    switch (s[0]) {
        case 'a': {
            type = MX_LEX_TYPE_AND;
            break;
        }
        case 'b': {
            type = MX_LEX_TYPE_BREAK;
            break;
        }
        case 'd': {
            type = MX_LEX_TYPE_DO;
            break;
        }
        case 'e': {
            if (len == 4) {
                type = MX_LEX_TYPE_ELSE;
            } else if (len == 6) {
                type = MX_LEX_TYPE_ELSEIF;
            } else {
                type = MX_LEX_TYPE_END;
            }
            break;
        }
        case 'f': {
            if (len == 5) {
                type = MX_LEX_TYPE_FALSE;
            } else if (len == 3) {
                type = MX_LEX_TYPE_FOR;
            } else {
                type = MX_LEX_TYPE_FUNCTION;
            }
            break;
        }
        case 'g': {
            type = MX_LEX_TYPE_GOTO;
            break;
        }
        case 'i': {
            if (s[1] == 'f') {
                type = MX_LEX_TYPE_IF;
            } else {
                type = MX_LEX_TYPE_IN;
            }
            break;
        }
        case 'l': {
            type = MX_LEX_TYPE_LOCAL;
            break;
        }
        case 'n': {
            if (s[1] == 'i') {
                type = MX_LEX_TYPE_NIL;
            } else {
                type = MX_LEX_TYPE_NOT;
            }
            break;
        }
        case 'o': {
            type = MX_LEX_TYPE_OR;
            break;
        }
        case 'r': {
            if (len > 2 && s[2] == 'p') {
                type = MX_LEX_TYPE_REPEAT;
            } else {
                type = MX_LEX_TYPE_RETURN;
            }
            break;
        }
        case 't': {
            if (s[1] == 'h') {
                type = MX_LEX_TYPE_THEN;
            } else {
                type = MX_LEX_TYPE_TRUE;
            }
            break;
        }
        case 'u': {
            type = MX_LEX_TYPE_UNTIL;
            break;
        }
        case 'w': {
            type = MX_LEX_TYPE_WHILE;
            break;
        }
        default: {
            return MX_LEX_TYPE_NAME;
        }
    }
    const char *word = mx_lex_type_names[type];
    if (strlen(word) == len && memcmp(word, s, len) == 0) {
        return type;
    } else {
        return MX_LEX_TYPE_NAME;
    }
}

mx_lex_scan_t mx_lex_scan_raw(mx_lex_t *lex) {
    const uint8_t *src = (const uint8_t *)lex->src;
    while (true) {
        uint32_t pos = lex->pos;
        while ((mx_lex_classes[src[pos]] & MX_LEX_CLASS_SPACE) != 0) {
            pos += 1;
        }
        lex->pos = pos;
        uint8_t c = mx_lex_at(lex, lex->pos);
        if (lex->pos >= lex->len) {
            return (mx_lex_scan_t) {
                .tok = (mx_lex_token_t) {
                    .start = lex->len,
                    .stop = lex->len,
                    .type = MX_LEX_TYPE_EOF,
                },
                .error = NULL,
            };
        } else if (c == '-' && mx_lex_at(lex, lex->pos + 1) == '-') {
            uint32_t open_start = lex->pos;
            lex->pos += 2;
            int64_t level;
            if (mx_lex_at(lex, lex->pos) == '[') {
                level = mx_lex_open_level(lex, lex->pos);
            } else {
                level = -1;
            }
            if (level >= 0) {
                lex->pos += (uint32_t)level + 2;
                mx_error_t *error = mx_lex_long_body(lex, open_start, level, "comment").error;
                if (error != NULL) {
                    return (mx_lex_scan_t) { .error = error };
                }
            } else {
                uint32_t end = lex->pos;
                while (end < lex->len && src[end] != '\n' && src[end] != '\r') {
                    end += 1;
                }
                lex->pos = end;
            }
        } else {
            break;
        }
    }
    uint32_t start = lex->pos;
    uint8_t c = mx_lex_at(lex, lex->pos);
    if (mx_lex_byte_is_name_start(c)) {
        uint32_t at = lex->pos + 1;
        while (mx_lex_byte_is_name(src[at])) {
            at += 1;
        }
        lex->pos = at;
        return (mx_lex_scan_t) {
            .tok = (mx_lex_token_t) {
                .start = start,
                .stop = at,
                .type = mx_lex_keyword(lex->src, start, at),
            },
            .error = NULL,
        };
    } else if (mx_lex_byte_is_digit(c) || (c == '.' && mx_lex_byte_is_digit(mx_lex_at(lex, lex->pos + 1)))) {
        return mx_lex_number(lex);
    } else if (c == '"' || c == '\'') {
        return mx_lex_short_string(lex);
    } else if (c == '[') {
        int64_t level = mx_lex_open_level(lex, lex->pos);
        if (level >= 0) {
            lex->pos += (uint32_t)level + 2;
            mx_lex_long_t body = mx_lex_long_body(lex, start, level, "string");
            if (body.error != NULL) {
                return (mx_lex_scan_t) { .error = body.error };
            }
            return mx_lex_scan_value(MX_LEX_TYPE_STRING, start, lex->pos, mx_lex_long_value(lex, body.content_start, body.content_stop));
        } else if (mx_lex_at(lex, lex->pos + 1) == '=') {
            uint32_t at = lex->pos + 1;
            while (mx_lex_at(lex, at) == '=') {
                at += 1;
            }
            return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, at, "invalid long string delimiter") };
        } else {
            lex->pos += 1;
            return (mx_lex_scan_t) {
                .tok = (mx_lex_token_t) {
                    .start = start,
                    .stop = start + 1,
                    .type = MX_LEX_TYPE_LBRACKET,
                },
                .error = NULL,
            };
        }
    } else {
        return mx_lex_operator(lex, start, c);
    }
}

mx_lex_scan_t mx_lex_operator(mx_lex_t *lex, uint32_t start, uint8_t c) {
    mx_lex_type_t type;
    uint8_t next = mx_lex_at(lex, lex->pos + 1);
    switch (c) {
        case '+': {
            type = MX_LEX_TYPE_PLUS;
            break;
        }
        case '-': {
            type = MX_LEX_TYPE_MINUS;
            break;
        }
        case '*': {
            type = MX_LEX_TYPE_STAR;
            break;
        }
        case '/': {
            if (next == '/') {
                type = MX_LEX_TYPE_SLASH2;
            } else {
                type = MX_LEX_TYPE_SLASH;
            }
            break;
        }
        case '%': {
            type = MX_LEX_TYPE_PERCENT;
            break;
        }
        case '^': {
            type = MX_LEX_TYPE_CARET;
            break;
        }
        case '#': {
            type = MX_LEX_TYPE_HASH;
            break;
        }
        case '&': {
            type = MX_LEX_TYPE_AMP;
            break;
        }
        case '~': {
            if (next == '=') {
                type = MX_LEX_TYPE_NE;
            } else {
                type = MX_LEX_TYPE_TILDE;
            }
            break;
        }
        case '|': {
            type = MX_LEX_TYPE_PIPE;
            break;
        }
        case '<': {
            if (next == '<') {
                type = MX_LEX_TYPE_SHL;
            } else if (next == '=') {
                type = MX_LEX_TYPE_LE;
            } else {
                type = MX_LEX_TYPE_LT;
            }
            break;
        }
        case '>': {
            if (next == '>') {
                type = MX_LEX_TYPE_SHR;
            } else if (next == '=') {
                type = MX_LEX_TYPE_GE;
            } else {
                type = MX_LEX_TYPE_GT;
            }
            break;
        }
        case '=': {
            if (next == '=') {
                type = MX_LEX_TYPE_EQ2;
            } else {
                type = MX_LEX_TYPE_ASSIGN;
            }
            break;
        }
        case '(': {
            type = MX_LEX_TYPE_LPAREN;
            break;
        }
        case ')': {
            type = MX_LEX_TYPE_RPAREN;
            break;
        }
        case '{': {
            type = MX_LEX_TYPE_LBRACE;
            break;
        }
        case '}': {
            type = MX_LEX_TYPE_RBRACE;
            break;
        }
        case ']': {
            type = MX_LEX_TYPE_RBRACKET;
            break;
        }
        case ';': {
            type = MX_LEX_TYPE_SEMI;
            break;
        }
        case ':': {
            if (next == ':') {
                type = MX_LEX_TYPE_LABEL;
            } else {
                type = MX_LEX_TYPE_COLON;
            }
            break;
        }
        case ',': {
            type = MX_LEX_TYPE_COMMA;
            break;
        }
        case '.': {
            if (next == '.' && mx_lex_at(lex, lex->pos + 2) == '.') {
                type = MX_LEX_TYPE_DOTS;
            } else if (next == '.') {
                type = MX_LEX_TYPE_CONCAT;
            } else {
                type = MX_LEX_TYPE_DOT;
            }
            break;
        }
        default: {
            if (c >= 0x21 && c <= 0x7e) {
                return (mx_lex_scan_t) { .error = mx_lex_error_slice(lex, start, start + 1, "unexpected symbol") };
            } else {
                mx_str_t *msg = mx_str_new();
                mx_str_format(msg, "unexpected symbol near '<\\%d>'", (int32_t)c);
                return (mx_lex_scan_t) { .error = mx_lex_error_finish(lex, msg, start) };
            }
        }
    }
    uint32_t stop = start + (uint32_t)strlen(mx_lex_type_names[type]);
    lex->pos = stop;
    return (mx_lex_scan_t) {
        .tok = (mx_lex_token_t) {
            .start = start,
            .stop = stop,
            .type = type,
        },
        .error = NULL,
    };
}

mx_lex_scan_t mx_lex_scan(mx_lex_t *lex) {
    mx_lex_scan_t got = mx_lex_scan_raw(lex);
    if (got.error == NULL && got.tok.type == MX_LEX_TYPE_NAME) {
        return (mx_lex_scan_t) {
            .tok = (mx_lex_token_t) {
                .start = got.tok.start,
                .stop = got.tok.stop,
                .name = mx_scratch_intern_range(lex->vm, &lex->src[got.tok.start], got.tok.stop - got.tok.start),
                .type = MX_LEX_TYPE_NAME,
            },
            .error = NULL,
        };
    } else {
        return got;
    }
}

mx_error_t *mx_lex_next(mx_lex_t *lex) {
    lex->last_stop = lex->tok.stop;
    if (lex->has_ahead) {
        lex->tok = lex->ahead;
        lex->has_ahead = false;
        return NULL;
    }
    mx_lex_scan_t got = mx_lex_scan(lex);
    if (got.error == NULL) {
        lex->tok = got.tok;
    }
    return got.error;
}

mx_lex_type_t mx_lex_peek(mx_lex_t *lex) {
    if (lex->has_ahead) {
        return lex->ahead.type;
    }
    uint32_t pos = lex->pos;
    mx_lex_scan_t got = mx_lex_scan(lex);
    if (got.error != NULL) {
        lex->pos = pos;
        return MX_LEX_TYPE_EOF;
    } else {
        lex->ahead = got.tok;
        lex->has_ahead = true;
        return got.tok.type;
    }
}

const char *mx_lex_tok_text(mx_lex_t *lex) {
    if (lex->tok.name != NULL) {
        return lex->tok.name;
    } else {
        return mx_scratch_intern_range(lex->vm, &lex->src[lex->tok.start], lex->tok.stop - lex->tok.start);
    }
}

bool mx_lex_tok_is(mx_lex_t *lex, const char *text) {
    size_t len = strlen(text);
    return lex->tok.stop - lex->tok.start == len && memcmp(&lex->src[lex->tok.start], text, len) == 0;
}

mx_error_t *mx_lex_error_expected(mx_lex_t *lex, mx_lex_type_t type) {
    if (type <= MX_LEX_TYPE_STRING) {
        return mx_lex_error_near(lex, "%s expected", mx_lex_type_name(type));
    } else {
        return mx_lex_error_near(lex, "'%s' expected", mx_lex_type_name(type));
    }
}

mx_error_t *mx_lex_error_match(mx_lex_t *lex, mx_lex_type_t what, mx_lex_type_t open, uint32_t open_at) {
    size_t line = mx_lex_line_of(lex, open_at);
    if (line == mx_lex_line_of(lex, lex->tok.start)) {
        return mx_lex_error_expected(lex, what);
    } else {
        return mx_lex_error_near(lex, "'%s' expected (to close '%s' at line %zu)", mx_lex_type_name(what), mx_lex_type_name(open), line);
    }
}

mx_lex_check_t mx_lex_check(mx_lex_t *lex, mx_lex_type_t type) {
    if (lex->tok.type == type) {
        return (mx_lex_check_t) {
            .error = mx_lex_next(lex),
            .is_match = true,
        };
    } else {
        return (mx_lex_check_t) {
            .error = NULL,
            .is_match = false,
        };
    }
}

mx_error_t *mx_lex_expect(mx_lex_t *lex, mx_lex_type_t type) {
    if (lex->tok.type != type) {
        return mx_lex_error_expected(lex, type);
    } else {
        return mx_lex_next(lex);
    }
}

mx_lex_name_t mx_lex_name(mx_lex_t *lex) {
    if (lex->tok.type != MX_LEX_TYPE_NAME) {
        return (mx_lex_name_t) {
            .error = mx_lex_error_expected(lex, MX_LEX_TYPE_NAME),
            .name = NULL,
            .at = lex->tok.start,
        };
    } else {
        const char *name = lex->tok.name;
        uint32_t at = lex->tok.start;
        return (mx_lex_name_t) {
            .error = mx_lex_next(lex),
            .name = name,
            .at = at,
        };
    }
}

bool mx_lex_is_callargs(mx_lex_t *lex) {
    return lex->tok.type == MX_LEX_TYPE_LPAREN
        || lex->tok.type == MX_LEX_TYPE_STRING
        || lex->tok.type == MX_LEX_TYPE_LBRACE;
}

bool mx_lex_is_global(mx_lex_t *lex) {
    if (lex->tok.type == MX_LEX_TYPE_NAME && mx_lex_tok_is(lex, "global")) {
        mx_lex_type_t after = mx_lex_peek(lex);
        return after == MX_LEX_TYPE_NAME || after == MX_LEX_TYPE_STAR || after == MX_LEX_TYPE_FUNCTION || after == MX_LEX_TYPE_LT;
    } else {
        return false;
    }
}

void mx_lex_init(mx_lex_t *lex, mx_t *vm, const mx_source_t *source, const char *src) {
    *lex = (mx_lex_t) {
        .vm = vm,
        .source = source,
        .src = src,
        .len = source->len,
        .scratch = mx_str_new(),
        .tok = (mx_lex_token_t) {
            .type = MX_LEX_TYPE_EOF,
        },
    };
}

void mx_lex_free(mx_lex_t *lex) {
    mx_str_free(lex->scratch);
}
