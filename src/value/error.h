
#pragma once

#if !defined(MX_HEADER_VALUE_ERROR)
#define MX_HEADER_VALUE_ERROR

#include "../vm/vm.h"

typedef struct mx_source_t mx_source_t;
typedef struct mx_varname_t mx_varname_t;
typedef struct mx_error_chunkid_t mx_error_chunkid_t;

typedef uint8_t mx_varhint_t;
enum {
    MX_VARHINT_NONE,
    MX_VARHINT_LOCAL,
    MX_VARHINT_GLOBAL,
    MX_VARHINT_FIELD,
    MX_VARHINT_METHOD,
    MX_VARHINT_UPVALUE,
    MX_VARHINT_CONSTANT,
    MX_VARHINT_FOR_ITERATOR,
};

typedef uint8_t mx_error_operand_t;
enum {
    MX_ERROR_OPERAND_NONE,
    MX_ERROR_OPERAND_LHS,
    MX_ERROR_OPERAND_RHS,
};

struct mx_source_t {
    const char *file;
    const uint32_t *lines;
    uint32_t len;
    uint32_t nlines;
};

struct mx_varname_t {
    const char *name;
    mx_varhint_t kind;
};

struct mx_location_range_t {
    const mx_source_t *source;
    uint32_t start;
    mx_varname_t names[2];
};

struct mx_error_t {
    mx_obj_head_t head;
    mx_kind_t payload_kind;
    mx_error_operand_t operand;
    bool is_raw: 1;
    bool is_handled: 1;
    bool is_hint_inline: 1;
    bool is_in_handler: 1;
    mx_val_t payload;
    void *gc_next;
};

struct mx_error_chunkid_t {
    char id[60];
};

extern const char *const mx_varhint_names[];

uint32_t mx_source_scan(const char *src, uint32_t len, uint32_t *lines);
const mx_source_t *mx_source_new(mx_t *vm, const char *file, const char *src, uint32_t len);
const mx_source_t *mx_source_of_lines(mx_t *vm, const char *file, const uint32_t *lines, uint32_t nlines, uint32_t len);
uint32_t mx_source_line(const mx_source_t *source, uint32_t byte);
mx_error_chunkid_t mx_error_chunkid(const char *file);
mx_error_t *mx_error_new(mx_t *vm, mx_obj_t payload);
mx_obj_t mx_error_raw_vformat(mx_t *vm, mx_error_operand_t operand, const char *fmt, va_list ap);
mx_obj_t mx_error_raw(mx_t *vm, const char *fmt, ...);
mx_obj_t mx_error_raw_operand(mx_t *vm, mx_error_operand_t operand, const char *fmt, ...);
mx_obj_t mx_error_value(mx_t *vm, mx_obj_t payload);
mx_obj_t mx_error_message(mx_t *vm, const mx_location_range_t *range, const char *fmt, ...);
void mx_error_where(mx_str_t *out, const mx_location_range_t *range);
void mx_error_locate(mx_t *vm, mx_error_t *err, const mx_location_range_t *range);
mx_obj_t mx_error_payload(mx_obj_t err);

#endif
