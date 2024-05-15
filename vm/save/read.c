
#include "save.h"

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
    int64_t x = 0;
    size_t shift = 0;
    while (true) {
        uint8_t buf = vm_save_read_byte(read);
        x += (buf & 0x7f) << shift;
        shift += 7;
        if (buf < 0x80) {
            if (shift < 64 && (buf & 0x40)) {
                return (int64_t)(x - ((int64_t)1 << shift));
            } else {
                return x;
            }
        }
    }
}

bool vm_save_read_is_done(vm_save_read_t *read) {
    return read->buf.read == read->buf.len;
}

vm_std_value_t vm_load_value(vm_save_t arg) {
    vm_save_read_t read = (vm_save_read_t){
        .buf.len = arg.len,
        .buf.bytes = arg.buf,
        .buf.read = 0,
        .values.len = 0,
        .values.ptr = NULL,
        .values.alloc = 0,
    };
    while (!vm_save_read_is_done(&read)) {
        size_t start = read.buf.read;
        vm_tag_t tag = (vm_tag_t) vm_save_read_byte(&read);
        vm_value_t value;
        switch (tag) {
            case VM_TAG_UNK: {
                break;
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
                value.i8 = (int8_t) vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_I16: {
                value.i16 = (int16_t) vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_FUN:
            case VM_TAG_I32: {
                value.i32 = (int32_t) vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_I64: {
                value.i64 = (int64_t) vm_save_read_sleb(&read);
                break;
            }
            case VM_TAG_F32: {
                uint32_t n = (uint32_t) vm_save_read_uleb(&read);
                value.f32 = *(float *)&n;
                break;
            }
            case VM_TAG_F64: {
                uint64_t n = vm_save_read_uleb(&read);
                value.f64 = *(double *)&n;
                break;
            }
            case VM_TAG_ERROR:
            case VM_TAG_STR: {
                uint64_t len = vm_save_read_uleb(&read);
                char *buf = vm_malloc(sizeof(char) * len);
                for (size_t i = 0; i < len; i++) {
                    buf[i] = vm_save_read_byte(&read);
                }
                value.str = buf;
                break;
            }
            case VM_TAG_FFI: {
                value.all = (void *) (size_t) vm_save_read_uleb(&read);
                break;
            }
            case VM_TAG_CLOSURE: {
                uint64_t len = vm_save_read_uleb(&read);
                vm_std_value_t *closure = vm_malloc(sizeof(vm_std_value_t) * (len + 1));
                closure[0] = (vm_std_value_t) {
                    .tag = VM_TAG_I32,
                    .value.i32 = (int32_t) (uint32_t) len,
                };
                closure += 1;
                for (size_t i = 0; i < len; i++) {
                    vm_save_read_uleb(&read);
                }
                value.closure = closure;
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
                fprintf(stderr, "unhandled object #%zu at [%zu]: type %zu\n", read.values.len, start, (size_t) tag);
                goto error;
            }
        }
        if (read.values.len + 1 >= read.values.alloc) {
            read.values.alloc = (read.values.len + 1) * 2;
            read.values.ptr = vm_realloc(read.values.ptr, sizeof(vm_save_value_t) * read.values.alloc);
        }
        read.values.ptr[read.values.len++] = (vm_save_value_t) {
            .start = start,
            .value = (vm_std_value_t) {
                .tag = tag,
                .value = value,
            },
        };
    }
    for (size_t i = 0; i < read.values.len; i++) {
        vm_save_value_t save = read.values.ptr[i];
        vm_value_t value = save.value.value;
        read.buf.read = save.start;
        vm_tag_t tag = (vm_tag_t) vm_save_read_byte(&read);
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
                for (size_t i = 0; i < real; i++) {
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
    return read.values.ptr[0].value;
error:;
    return (vm_std_value_t) {.tag = VM_TAG_NIL};
}
