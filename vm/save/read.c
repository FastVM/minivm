
#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../backend/backend.h"
#include "value.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);

struct vm_save_read_t;
typedef struct vm_save_read_t vm_save_read_t;

struct vm_save_value_t;
typedef struct vm_save_value_t vm_save_value_t;

struct vm_save_value_t {
    size_t start;
    vm_std_value_t value;
};

struct vm_save_read_t {
    struct {
        size_t len;
        vm_save_value_t *ptr;
        size_t alloc;
    } values;

    struct {
        size_t len;
        uint8_t *bytes;
        size_t read;
    } buf;
};

static uint8_t vm_save_read_byte(vm_save_read_t *read) {
    return read->buf.bytes[read->buf.read++];
}

static uint64_t vm_save_read_uleb(vm_save_read_t *read) {
    uint64_t x = 0;
    size_t shift = 0;
    while (true) {
        uint8_t buf = vm_save_read_byte(read);
        x |= (uint64_t)(buf & 0x7f) << shift;
        if (buf < 0x80) {
            return x;
        }
        shift += 7;
    }
}

static int64_t vm_save_read_sleb(vm_save_read_t *read) {
    int64_t result = 0;
    uint8_t shift = 0;
    while (true) {
        uint8_t byte = vm_save_read_byte(read);
        result |= (byte & 0x7f) << shift;
        shift += 7;
        if ((0x80 & byte) == 0) {
            if (shift < 64 && (byte & 0x40) != 0) {
                return result | -(1 << shift);
            }
            return result;
        }
    }
}

bool vm_save_read_is_done(vm_save_read_t *read) {
    return read->buf.read == read->buf.len;
}

vm_save_loaded_t vm_load_value(vm_config_t *config, vm_save_t arg) {
    vm_save_read_t read = (vm_save_read_t){
        .buf.len = arg.len,
        .buf.bytes = arg.buf,
        .buf.read = 0,
        .values.len = 0,
        .values.ptr = NULL,
        .values.alloc = 0,
    };
    while (true) {
        size_t start = read.buf.read;
        vm_tag_t tag = (vm_tag_t)vm_save_read_byte(&read);
        vm_value_t value;
        // printf("object #%zu at [0x%zX] with type %zu\n", read.values.len, start, (size_t) tag);
        switch (tag) {
            case VM_TAG_UNK: {
                goto outer;
            }
            case VM_TAG_NIL: {
                value.all = NULL;
                break;
            }
            case VM_TAG_BOOL: {
                value.b = vm_save_read_byte(&read) != 0;
                break;
            }
            case VM_TAG_I8: {
                value.i8 = (int8_t)vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_I16: {
                value.i16 = (int16_t)vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_FUN:
            case VM_TAG_I32: {
                value.i32 = (int32_t)vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_I64: {
                value.i64 = (int64_t)vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_F32: {
                uint32_t n = (uint32_t)vm_save_read_uleb(&read);
                value.i32 = (int32_t)n;
                break;
            }
            case VM_TAG_F64: {
                uint64_t n = vm_save_read_uleb(&read);
                value.i64 = (int64_t)n;
                break;
            }
            case VM_TAG_ERROR:
            case VM_TAG_STR: {
                uint64_t len = vm_save_read_uleb(&read);
                char *buf = vm_malloc(sizeof(char) * (len + 1));
                for (size_t i = 0; i < len; i++) {
                    buf[i] = vm_save_read_byte(&read);
                }
                buf[len] = '\0';
                value.str = buf;
                break;
            }
            case VM_TAG_FFI: {
                size_t id = vm_save_read_uleb(&read);
                value.all = NULL;
                for (vm_externs_t *cur = config->externs; cur; cur = cur->last) {
                    if (cur->id == id) {
                        value.all = cur->value;
                    }
                }
                break;
            }
            case VM_TAG_CLOSURE: {
                uint64_t len = vm_save_read_uleb(&read);
                vm_std_value_t *closure = vm_malloc(sizeof(vm_std_value_t) * (len + 1));
                closure[0] = (vm_std_value_t){
                    .tag = VM_TAG_I32,
                    .value.i32 = (int32_t)(uint32_t)len,
                };
                closure += 1;
                for (size_t i = 0; i < len; i++) {
                    vm_save_read_uleb(&read);
                }
                value.closure = closure;
                break;
            }
            case VM_TAG_TAB: {
                uint64_t real = vm_save_read_uleb(&read);
                vm_table_t *table = vm_table_new();
                for (size_t i = 0; i < real; i++) {
                    vm_save_read_uleb(&read);
                    vm_save_read_uleb(&read);
                }
                value.table = table;
                break;
            }
            default: {
                fprintf(stderr, "unhandled object #%zu at [0x%zX]: type %zu\n", read.values.len, start, (size_t)tag);
                goto error;
            }
        }
        if (read.values.len + 1 >= read.values.alloc) {
            read.values.alloc = (read.values.len + 1) * 2;
            read.values.ptr = vm_realloc(read.values.ptr, sizeof(vm_save_value_t) * read.values.alloc);
        }
        read.values.ptr[read.values.len++] = (vm_save_value_t){
            .start = start,
            .value = (vm_std_value_t){
                .tag = tag,
                .value = value,
            },
        };
    }
outer:;
    size_t loc = read.buf.read;
    for (size_t i = 0; i < read.values.len; i++) {
        vm_save_value_t save = read.values.ptr[i];
        vm_value_t value = save.value.value;
        read.buf.read = save.start;
        vm_tag_t tag = (vm_tag_t)vm_save_read_byte(&read);
        switch (tag) {
            case VM_TAG_CLOSURE: {
                uint64_t len = vm_save_read_uleb(&read);
                vm_std_value_t *closure = value.closure;
                for (uint64_t i = 0; i < len; i++) {
                    size_t value_index = vm_save_read_uleb(&read);
                    closure[i] = read.values.ptr[value_index].value;
                }
                break;
            }
            case VM_TAG_TAB: {
                uint64_t real = vm_save_read_uleb(&read);
                vm_table_t *table = value.table;
                for (uint64_t i = 0; i < real; i++) {
                    size_t key_index = vm_save_read_uleb(&read);
                    size_t value_index = vm_save_read_uleb(&read);
                    VM_TABLE_SET_VALUE(table, read.values.ptr[key_index].value, read.values.ptr[value_index].value);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    read.buf.read = loc;
    vm_blocks_t *blocks = vm_malloc(sizeof(vm_blocks_t));
    blocks->len = 0;
    blocks->alloc = 0;
    blocks->blocks = NULL;
    blocks->srcs = NULL;
    uint64_t nsrcs = vm_save_read_uleb(&read);
    for (uint64_t i = 0; i < nsrcs; i++) {
        uint64_t len = vm_save_read_uleb(&read);
        char *src = vm_malloc(sizeof(char) * (len + 1));
        for (uint64_t j = 0; j < len; j++) {
            src[j] = (char)vm_save_read_byte(&read);
        }
        src[len] = '\0';
        vm_ast_node_t node = vm_lang_lua_parse(config, src);
        vm_blocks_add_src(blocks, src);
        vm_ast_comp_more(node, blocks);
    }
    return (vm_save_loaded_t){
        .blocks = blocks,
        .env = read.values.ptr[0].value,
    };
error:;
    return (vm_save_loaded_t){
        .blocks = NULL,
        .env = (vm_std_value_t){.tag = VM_TAG_NIL},
    };
}
