
#include "save.h"

struct vm_save_read_t;
typedef struct vm_save_read_t vm_save_read_t;

struct vm_save_read_t {
    struct {
        size_t len;
        vm_std_value_t *ptr;
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
    };
    while (!vm_save_read_is_done(&read)) {
        vm_tag_t type = (vm_tag_t) vm_save_read_byte(&read);
        vm_value_t val;
        switch (type) {
            default: {
                fprintf("unhandled object %zu at %zu\n", read.values.len, read.buf.read);
                break;
            }
        }
    }
    return read.values.ptr[0];
}
