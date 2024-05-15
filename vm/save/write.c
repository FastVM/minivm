
#include "save.h"

struct vm_save_write_t;
typedef struct vm_save_write_t vm_save_write_t;

struct vm_save_write_t {
    struct {
        size_t len;
        uint8_t *bytes;
        size_t alloc;
    } buf;

    struct {
        size_t read;
        size_t write;
        vm_std_value_t *buf;
        size_t alloc;
    } values;
};

static size_t vm_save_write_push(vm_save_write_t *write, vm_std_value_t value) {
    for (size_t i = 0; i < write->values.write; i++) {
        if (vm_value_eq(write->values.buf[i], value)) {
            return i;
        }
    }
    size_t index = write->values.write++;
    if (index + 1 >= write->values.alloc) {
        write->values.alloc = (index + 1) * 2;
        write->values.buf = vm_realloc(write->values.buf, sizeof(vm_std_value_t) * write->values.alloc);
    }
    write->values.buf[index] = value;
    return index;
}

static vm_std_value_t vm_save_write_shift(vm_save_write_t *write) {
    return write->values.buf[write->values.read++];
}

static bool vm_save_write_is_done(vm_save_write_t *write) {
    return write->values.read == write->values.write;
}

static void vm_save_write_byte(vm_save_write_t *write, uint8_t value) {
    size_t index = write->buf.len++;
    if (index + 1 >= write->buf.alloc) {
        write->buf.alloc = (index + 1) * 2;
        write->buf.bytes = vm_realloc(write->buf.bytes, sizeof(uint8_t) * write->buf.alloc);
    }
    write->buf.bytes[index] = value;
}

void vm_save_write_uleb(vm_save_write_t *write, uint64_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        vm_save_write_byte(write, byte);
    } while (value != 0);
}

void vm_save_write_sleb(vm_save_write_t *write, int64_t value) {
    size_t len = 0;
    uint8_t result[10];
    while (true) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if ((value == 0 && (byte & 0x40) == 0) || (value == -1 && (byte & 0x40) != 0)) {
            vm_save_write_uleb(write, byte);
            break;
        }
        vm_save_write_uleb(write, byte | 0x80);
    }
}

vm_save_t vm_save_value(vm_std_value_t arg) {
    vm_save_write_t write = (vm_save_write_t){
        .buf.len = 0,
        .buf.bytes = NULL,
        .buf.alloc = 0,
        .values.write = 0,
        .values.read = 0,
        .values.buf = NULL,
        .values.alloc = 0,
    };
    vm_save_write_push(&write, arg);
    while (!vm_save_write_is_done(&write)) {
        vm_std_value_t value = vm_save_write_shift(&write);
        vm_save_write_byte(&write, value.tag);
        switch (value.tag) {
            case VM_TAG_UNK: {
                break;
            }
            case VM_TAG_NIL: {
                break;
            }
            case VM_TAG_BOOL: {
                vm_save_write_byte(&write, value.value.b ? 1 : 0);
                break;
            }
            case VM_TAG_I8: {
                vm_save_write_sleb(&write, (int64_t) value.value.i8);
                break;
            }
            case VM_TAG_I16: {
                vm_save_write_sleb(&write, (int64_t) value.value.i16);
                break;
            }
            case VM_TAG_FUN:
            case VM_TAG_I32: {
                vm_save_write_sleb(&write, (int64_t) value.value.i32);
                break;
            }
            case VM_TAG_I64: {
                vm_save_write_sleb(&write, value.value.i64);
                break;
            }
            case VM_TAG_F32: {
                vm_save_write_uleb(&write, (uint64_t) *(uint32_t *) &value.value.f64);
                break;
            }
            case VM_TAG_F64: {
                vm_save_write_uleb(&write, *(uint64_t *) &value.value.f64);
                break;
            }
            case VM_TAG_ERROR:
            case VM_TAG_STR: {
                const char *buf = value.value.str;
                size_t len = strlen(buf) + 1;
                vm_save_write_uleb(&write, (uint64_t) len);
                for (size_t i = 0; i < len; i++) {
                    vm_save_write_byte(&write, (uint8_t) buf[i]);
                }
                break;
            }
            case VM_TAG_CLOSURE: {
                vm_std_value_t *closure = value.value.closure;
                uint32_t len = closure[-1].value.i32;
                vm_save_write_uleb(&write, (uint64_t) len);
                for (uint32_t i = 0; i < len; i++) {
                    size_t ent = vm_save_write_push(&write, closure[i]);
                    vm_save_write_uleb(&write, (uint64_t) ent);
                }
                break;
            }
            case VM_TAG_TAB: {
                vm_table_t *table = value.value.table;
                uint32_t len = (uint32_t) 1 << table->alloc;
                size_t real = 0;
                for (size_t i = 0; i < len; i++) {
                    vm_pair_t pair = table->pairs[i];
                    if (pair.key_tag != VM_TAG_UNK) {
                        vm_save_write_push(&write, (vm_std_value_t) {.tag = pair.key_tag, .value = pair.key_val});
                        vm_save_write_push(&write, (vm_std_value_t) {.tag = pair.val_tag, .value = pair.val_val});
                        real += 1;
                    }
                }
                vm_save_write_uleb(&write, (uint64_t) real);
                for (size_t i = 0; i < len; i++) {
                    vm_pair_t pair = table->pairs[i];
                    if (pair.key_tag != VM_TAG_UNK) {
                        size_t key = vm_save_write_push(&write, (vm_std_value_t) {.tag = pair.key_tag, .value = pair.key_val});
                        size_t value = vm_save_write_push(&write, (vm_std_value_t) {.tag = pair.val_tag, .value = pair.val_val});
                        vm_save_write_uleb(&write, (uint64_t) key);
                        vm_save_write_uleb(&write, (uint64_t) value);
                    }
                }
                break;
            }
            case VM_TAG_FFI: {
                vm_save_write_uleb(&write, (uint64_t) (size_t) value.value.all);
                break;
            }
            default: {
                vm_io_buffer_t *buf = vm_io_buffer_new();
                vm_io_debug(buf, 0, "error unhandled: ", value, NULL);
                fprintf(stderr, "%.*s", (int) buf->len, buf->buf);
                break;
            }
        }
    }
    return (vm_save_t){
        .len = write.buf.len,
        .buf = write.buf.bytes,
    };
}

vm_save_t vm_save_load(FILE *in) {
    size_t nalloc = 512;
    uint8_t *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    while (true) {
        if (nops + 256 >= nalloc) {
            nalloc = (nops + 256) * 2;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
        size = fread(&ops[nops], 1, 256, in);
        nops += size;
        if (size < 256) {
            break;
        }
    }
    return (vm_save_t) {
        .len = nops,
        .buf = ops,
    };
}
