
#include "../ast/ast.h"
#include "../ast/comp.h"
#include "../backend/backend.h"
#include "../gc.h"
#include "./value.h"

vm_ast_node_t vm_lang_lua_parse(vm_t *vm, const char *str, const char *file);

struct vm_save_read_t;
typedef struct vm_save_read_t vm_save_read_t;

struct vm_save_value_t;
typedef struct vm_save_value_t vm_save_value_t;

struct vm_save_value_t {
    size_t start;
    vm_obj_t value;
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
    __int128_t result = 0;
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

void vm_load_value(vm_t *vm, vm_save_t save) {
    vm_save_read_t read = (vm_save_read_t){
        .buf.len = save.len,
        .buf.bytes = save.buf,
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
            case VM_TAG_STR: {
                uint64_t len = vm_save_read_uleb(&read);
                vm_io_buffer_t *buf = vm_io_buffer_new();
                for (size_t i = 0; i < len; i++) {
                    vm_io_buffer_format(buf, "%c", vm_save_read_byte(&read));
                }
                value.str = buf;
                break;
            }
            case VM_TAG_FFI: {
                size_t id = vm_save_read_uleb(&read);
                value.all = NULL;
                for (vm_externs_t *cur = vm->externs; cur; cur = cur->last) {
                    if (cur->id == id) {
                        value.all = cur->value;
                    }
                }
                break;
            }
            case VM_TAG_CLOSURE: {
                uint64_t len = vm_save_read_uleb(&read);
                vm_closure_t *closure = vm_malloc(sizeof(vm_closure_t) + sizeof(vm_obj_t) * len);
                closure->len = len;
                for (size_t i = 0; i < len; i++) {
                    vm_save_read_uleb(&read);
                }
                value.closure = closure;
                break;
            }
            case VM_TAG_TAB: {
                uint64_t real = vm_save_read_uleb(&read);
                vm_table_t *table = vm_table_new(vm);
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
            .value = (vm_obj_t){
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
                vm_closure_t *closure = value.closure;
                for (uint64_t i = 0; i < len; i++) {
                    size_t value_index = vm_save_read_uleb(&read);
                    closure->values[i] = read.values.ptr[value_index].value;
                }
                vm_gc_add(vm, (vm_obj_t) {.tag = tag, .value = value});
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
                vm_gc_add(vm, (vm_obj_t) {.tag = tag, .value = value});
                break;
            }
            default: {
                break;
            }
        }
    }
    read.buf.read = loc;
    vm->blocks = vm_malloc(sizeof(vm_blocks_t));
    *vm->blocks = (vm_blocks_t) { 0 };
    vm->std = read.values.ptr[0].value;
    uint64_t nsrcs = vm_save_read_uleb(&read);
    for (uint64_t i = 0; i < nsrcs; i++) {
        char *file;
        {
            uint64_t len = vm_save_read_uleb(&read);
            file = vm_malloc(sizeof(char) * (len + 1));
            for (uint64_t j = 0; j < len; j++) {
                file[j] = (char)vm_save_read_byte(&read);
            }
            file[len] = '\0';
        }
        char *src;
        {
            uint64_t len = vm_save_read_uleb(&read);
            src = vm_malloc(sizeof(char) * (len + 1));
            for (uint64_t j = 0; j < len; j++) {
                src[j] = (char)vm_save_read_byte(&read);
            }
            src[len] = '\0';
        }
        vm_compile(vm, src, file);
    }
error:;
}
