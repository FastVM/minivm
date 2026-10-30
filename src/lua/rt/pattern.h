
#pragma once

#if !defined(MX_HEADER_LUA_RT_PATTERN)
#define MX_HEADER_LUA_RT_PATTERN

#include "../../vm/vm.h"

#define MX_PATTERN_MAX_CAPTURES 32
#define MX_PATTERN_ERROR_CAP 64
#define MX_PATTERN_MAX_DEPTH 200
#define MX_PATTERN_ITEMS_INLINE 32
#define MX_PATTERN_CACHE_WAYS 16

typedef uint8_t mx_pattern_status_t;
enum {
    MX_PATTERN_NOMATCH,
    MX_PATTERN_MATCHED,
    MX_PATTERN_ERROR,
};

typedef uint8_t mx_pattern_op_t;
enum {
    MX_PATTERN_OP_ONE,
    MX_PATTERN_OP_OPEN,
    MX_PATTERN_OP_POSITION,
    MX_PATTERN_OP_CLOSE,
    MX_PATTERN_OP_BACKREF,
    MX_PATTERN_OP_BALANCE,
    MX_PATTERN_OP_FRONTIER,
    MX_PATTERN_OP_END,
    MX_PATTERN_OP_FAIL,
    MX_PATTERN_OP_DONE,
};

typedef uint8_t mx_pattern_rep_t;
enum {
    MX_PATTERN_REP_ONE,
    MX_PATTERN_REP_OPT,
    MX_PATTERN_REP_STAR,
    MX_PATTERN_REP_PLUS,
    MX_PATTERN_REP_LAZY,
};

typedef uint8_t mx_pattern_choice_kind_t;
enum {
    MX_PATTERN_CHOICE_SKIP,
    MX_PATTERN_CHOICE_GREEDY,
    MX_PATTERN_CHOICE_LAZY,
};

typedef struct mx_pattern_capture_t mx_pattern_capture_t;
typedef struct mx_pattern_match_t mx_pattern_match_t;
typedef struct mx_pattern_captures_t mx_pattern_captures_t;
typedef struct mx_pattern_result_t mx_pattern_result_t;
typedef struct mx_pattern_item_t mx_pattern_item_t;
typedef struct mx_pattern_prog_t mx_pattern_prog_t;
typedef struct mx_pattern_choice_t mx_pattern_choice_t;
typedef struct mx_pattern_undo_t mx_pattern_undo_t;
typedef struct mx_pattern_run_t mx_pattern_run_t;
typedef struct mx_pattern_set_end_t mx_pattern_set_end_t;
typedef struct mx_pattern_cached_t mx_pattern_cached_t;
typedef struct mx_pattern_cache_t mx_pattern_cache_t;

struct mx_pattern_capture_t {
    uint32_t start;
    uint32_t len;
    bool is_position: 1;
    bool is_closed: 1;
};

struct mx_pattern_captures_t {
    mx_pattern_capture_t at[MX_PATTERN_MAX_CAPTURES];
    char error[MX_PATTERN_ERROR_CAP];
};

struct mx_pattern_match_t {
    const mx_pattern_capture_t *captures;
    uint32_t start;
    uint32_t stop;
    uint32_t len;
    bool is_unfinished: 1;
};

struct mx_pattern_result_t {
    union {
        mx_pattern_match_t match;
        const char *error;
    };
    mx_pattern_status_t status;
};

struct mx_pattern_item_t {
    uint64_t set[4];
    const char *error;
    uint8_t a;
    uint8_t b;
    mx_pattern_op_t op;
    mx_pattern_rep_t rep;
};

struct mx_pattern_prog_t {
    mx_pattern_item_t *items;
    uint32_t len;
    uint32_t ncap;
    int32_t first_byte;
    mx_pattern_item_t inline_items[MX_PATTERN_ITEMS_INLINE];
};

struct mx_pattern_cached_t {
    char pat[MX_PATTERN_ITEMS_INLINE];
    size_t len;
    uint32_t busy;
    mx_pattern_prog_t prog;
};

struct mx_pattern_cache_t {
    uint64_t sets[26][4];
    uint32_t ready;
    mx_pattern_cached_t progs[MX_PATTERN_CACHE_WAYS];
};

struct mx_pattern_choice_t {
    uint32_t item;
    uint32_t lo;
    uint32_t at;
    uint32_t undo;
    mx_pattern_choice_kind_t kind;
};

struct mx_pattern_undo_t {
    mx_pattern_capture_t saved;
    uint32_t idx;
};

struct mx_pattern_run_t {
    const mx_pattern_prog_t *prog;
    const uint8_t *src;
    size_t len;
    mx_pattern_capture_t *caps;
    mx_pattern_choice_t *choices;
    mx_pattern_undo_t *undos;
    const char *error;
    int32_t error_arg;
};

struct mx_pattern_set_end_t {
    size_t next;
    bool is_closed: 1;
};

static inline bool mx_pattern_is_class(int32_t lower) {
    switch (lower) {
        case 'a':
        case 'c':
        case 'd':
        case 'g':
        case 'l':
        case 'p':
        case 's':
        case 'u':
        case 'w':
        case 'x':
        case 'z': {
            return true;
        }
        default: {
            return false;
        }
    }
}

static inline bool mx_pattern_class_test(int32_t lower, int32_t c) {
    switch (lower) {
        case 'a': {
            return isalpha(c) != 0;
        }
        case 'c': {
            return iscntrl(c) != 0;
        }
        case 'd': {
            return isdigit(c) != 0;
        }
        case 'g': {
            return isgraph(c) != 0;
        }
        case 'l': {
            return islower(c) != 0;
        }
        case 'p': {
            return ispunct(c) != 0;
        }
        case 's': {
            return isspace(c) != 0;
        }
        case 'u': {
            return isupper(c) != 0;
        }
        case 'w': {
            return isalnum(c) != 0;
        }
        case 'x': {
            return isxdigit(c) != 0;
        }
        default: {
            return c == 0;
        }
    }
}

bool mx_pattern_set_has(const uint64_t *set, uint8_t c);
void mx_pattern_set_add(uint64_t *set, uint8_t c);
void mx_pattern_set_escape(mx_pattern_cache_t *cache, uint64_t *set, uint8_t cl);
mx_pattern_set_end_t mx_pattern_set_parse(mx_pattern_cache_t *cache, uint64_t *set, const char *pat, size_t plen, size_t at);
mx_pattern_item_t *mx_pattern_emit(mx_pattern_prog_t *prog, mx_pattern_op_t op);
void mx_pattern_emit_fail(mx_pattern_prog_t *prog, const char *error, uint8_t arg);
void mx_pattern_compile(mx_pattern_cache_t *cache, mx_pattern_prog_t *prog, const char *pat, size_t plen);
const mx_pattern_prog_t *mx_pattern_prog(mx_pattern_cache_t *cache, mx_pattern_prog_t *own, const char *pat, size_t plen);
const mx_pattern_prog_t *mx_pattern_prog_hold(mx_pattern_cache_t *cache, mx_pattern_prog_t *own, const char *pat, size_t plen);
void mx_pattern_prog_release(mx_pattern_cache_t *cache, const mx_pattern_prog_t *prog);
mx_pattern_cached_t *mx_pattern_cached_of(mx_pattern_cache_t *cache, const mx_pattern_prog_t *prog);
void mx_pattern_prog_free(mx_pattern_prog_t *prog);
void mx_pattern_save(mx_pattern_run_t *r, uint32_t *nundo, uint32_t idx);
int64_t mx_pattern_exec(mx_pattern_run_t *r, size_t start);
mx_pattern_result_t mx_pattern_search(mx_pattern_captures_t *scratch, const mx_pattern_prog_t *prog, const char *src, size_t src_len, size_t init, bool anchored);
mx_pattern_result_t mx_pattern_match_from(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init, bool anchored);
mx_pattern_result_t mx_pattern_match_unanchored(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init);
mx_pattern_result_t mx_pattern_match(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init);

#endif
