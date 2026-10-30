
#pragma once

#if !defined(MX_HEADER_LUA_FRONT_LEX)
#define MX_HEADER_LUA_FRONT_LEX

#include "../../value/error.h"
#include "../../value/string.h"
#include "../../vm/vm.h"

typedef uint8_t mx_lex_class_t;
enum {
    MX_LEX_CLASS_SPACE = 1,
    MX_LEX_CLASS_NAME_START = 2,
    MX_LEX_CLASS_DIGIT = 4,
};

typedef struct mx_lex_t mx_lex_t;
typedef struct mx_lex_token_t mx_lex_token_t;
typedef struct mx_lex_long_t mx_lex_long_t;
typedef struct mx_lex_check_t mx_lex_check_t;
typedef struct mx_lex_name_t mx_lex_name_t;
typedef struct mx_lex_scan_t mx_lex_scan_t;

typedef uint8_t mx_lex_type_t;
enum {
    MX_LEX_TYPE_EOF,
    MX_LEX_TYPE_NAME,
    MX_LEX_TYPE_NUMBER,
    MX_LEX_TYPE_STRING,
    MX_LEX_TYPE_AND,
    MX_LEX_TYPE_BREAK,
    MX_LEX_TYPE_DO,
    MX_LEX_TYPE_ELSE,
    MX_LEX_TYPE_ELSEIF,
    MX_LEX_TYPE_END,
    MX_LEX_TYPE_FALSE,
    MX_LEX_TYPE_FOR,
    MX_LEX_TYPE_FUNCTION,
    MX_LEX_TYPE_GOTO,
    MX_LEX_TYPE_IF,
    MX_LEX_TYPE_IN,
    MX_LEX_TYPE_LOCAL,
    MX_LEX_TYPE_NIL,
    MX_LEX_TYPE_NOT,
    MX_LEX_TYPE_OR,
    MX_LEX_TYPE_REPEAT,
    MX_LEX_TYPE_RETURN,
    MX_LEX_TYPE_THEN,
    MX_LEX_TYPE_TRUE,
    MX_LEX_TYPE_UNTIL,
    MX_LEX_TYPE_WHILE,
    MX_LEX_TYPE_PLUS,
    MX_LEX_TYPE_MINUS,
    MX_LEX_TYPE_STAR,
    MX_LEX_TYPE_SLASH,
    MX_LEX_TYPE_SLASH2,
    MX_LEX_TYPE_PERCENT,
    MX_LEX_TYPE_CARET,
    MX_LEX_TYPE_HASH,
    MX_LEX_TYPE_AMP,
    MX_LEX_TYPE_TILDE,
    MX_LEX_TYPE_PIPE,
    MX_LEX_TYPE_SHL,
    MX_LEX_TYPE_SHR,
    MX_LEX_TYPE_EQ2,
    MX_LEX_TYPE_NE,
    MX_LEX_TYPE_LE,
    MX_LEX_TYPE_GE,
    MX_LEX_TYPE_LT,
    MX_LEX_TYPE_GT,
    MX_LEX_TYPE_ASSIGN,
    MX_LEX_TYPE_LPAREN,
    MX_LEX_TYPE_RPAREN,
    MX_LEX_TYPE_LBRACE,
    MX_LEX_TYPE_RBRACE,
    MX_LEX_TYPE_LBRACKET,
    MX_LEX_TYPE_RBRACKET,
    MX_LEX_TYPE_SEMI,
    MX_LEX_TYPE_COLON,
    MX_LEX_TYPE_LABEL,
    MX_LEX_TYPE_COMMA,
    MX_LEX_TYPE_DOT,
    MX_LEX_TYPE_CONCAT,
    MX_LEX_TYPE_DOTS,
};

struct mx_lex_token_t {
    uint32_t start;
    uint32_t stop;
    union {
        mx_val_t val;
        const char *name;
    };
    mx_kind_t kind;
    mx_lex_type_t type;
};

struct mx_lex_scan_t {
    mx_lex_token_t tok;
    mx_error_t *error;
};

struct mx_lex_long_t {
    mx_error_t *error;
    uint32_t content_start;
    uint32_t content_stop;
};

struct mx_lex_t {
    mx_t *vm;
    const mx_source_t *source;
    const char *src;
    uint32_t len;
    uint32_t pos;
    uint32_t last_stop;
    mx_lex_token_t tok;
    mx_lex_token_t ahead;
    mx_str_t *scratch;
    bool has_ahead: 1;
};

struct mx_lex_check_t {
    mx_error_t *error;
    bool is_match: 1;
};

struct mx_lex_name_t {
    mx_error_t *error;
    const char *name;
    uint32_t at;
};

extern const char *const mx_lex_type_names[];
extern const mx_lex_class_t mx_lex_classes[256];

mx_obj_t mx_lex_tok_value(const mx_lex_token_t *tok);
mx_lex_scan_t mx_lex_scan_value(mx_lex_type_t type, uint32_t start, uint32_t stop, mx_obj_t v);
bool mx_lex_byte_is_name_start(uint8_t c);
bool mx_lex_byte_is_name(uint8_t c);
bool mx_lex_byte_is_digit(uint8_t c);
bool mx_lex_byte_is_hex(uint8_t c);
bool mx_lex_byte_is_space(uint8_t c);
uint64_t mx_lex_hex_value(uint8_t c);
mx_error_t *mx_lex_error_finish(mx_lex_t *lex, mx_str_t *msg, uint32_t at);
uint8_t mx_lex_at(mx_lex_t *lex, uint32_t pos);
size_t mx_lex_line_of(mx_lex_t *lex, uint32_t pos);
int64_t mx_lex_open_level(mx_lex_t *lex, uint32_t pos);
mx_lex_long_t mx_lex_long_body(mx_lex_t *lex, uint32_t open_start, int64_t level, const char *what);
mx_obj_t mx_lex_long_value(mx_lex_t *lex, uint32_t start, uint32_t stop);
void mx_lex_utf8_encode(mx_str_t *buf, uint64_t cp);
mx_error_t *mx_lex_escape_error(mx_lex_t *lex, uint32_t start, const char *msg);
mx_error_t *mx_lex_escape(mx_lex_t *lex, mx_str_t *buf, uint32_t start);
mx_lex_scan_t mx_lex_short_string(mx_lex_t *lex);
mx_lex_scan_t mx_lex_number(mx_lex_t *lex);
mx_lex_type_t mx_lex_keyword(const char *src, uint32_t start, uint32_t stop);
mx_lex_scan_t mx_lex_operator(mx_lex_t *lex, uint32_t start, uint8_t c);
mx_lex_scan_t mx_lex_scan_raw(mx_lex_t *lex);
mx_lex_scan_t mx_lex_scan(mx_lex_t *lex);

void mx_lex_init(mx_lex_t *lex, mx_t *vm, const mx_source_t *source, const char *src);
void mx_lex_free(mx_lex_t *lex);
mx_error_t *mx_lex_next(mx_lex_t *lex);
mx_lex_type_t mx_lex_peek(mx_lex_t *lex);
const char *mx_lex_type_name(mx_lex_type_t type);
mx_error_t *mx_lex_error_slice(mx_lex_t *lex, uint32_t start, uint32_t stop, const char *fmt, ...);
mx_error_t *mx_lex_error_plain(mx_lex_t *lex, const char *fmt, ...);
mx_error_t *mx_lex_error_near(mx_lex_t *lex, const char *fmt, ...);
const char *mx_lex_tok_text(mx_lex_t *lex);
bool mx_lex_tok_is(mx_lex_t *lex, const char *text);
mx_error_t *mx_lex_error_expected(mx_lex_t *lex, mx_lex_type_t type);
mx_error_t *mx_lex_error_match(mx_lex_t *lex, mx_lex_type_t what, mx_lex_type_t open, uint32_t open_at);
mx_lex_check_t mx_lex_check(mx_lex_t *lex, mx_lex_type_t type);
mx_error_t *mx_lex_expect(mx_lex_t *lex, mx_lex_type_t type);
mx_lex_name_t mx_lex_name(mx_lex_t *lex);
bool mx_lex_is_callargs(mx_lex_t *lex);
bool mx_lex_is_global(mx_lex_t *lex);

#endif
