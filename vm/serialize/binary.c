
#include "binary.h"

void vm_serialize_binary_buf_put(vm_serialize_binary_buf_t *buf, uint8_t byte) {
    if (buf->len + 1 >= buf->alloc) {
        buf->alloc = (buf->len + 16) * 2;
        buf->mem = vm_realloc(buf->mem, sizeof(uint8_t) * buf->alloc);
    }
    buf->mem[buf->len++] = byte;
}

void vm_serialize_binary_buf_putn(vm_serialize_binary_buf_t *buf, size_t len, void *inbuf) {
    uint8_t *bytes = inbuf;
    for (size_t i = 0; i < len; i++) {
        vm_serialize_binary_buf_put(buf, bytes[i]);
    }
}

void vm_serialize_binary_buf_leb128(vm_serialize_binary_buf_t *buf, int64_t value) {
    bool negative = (value < 0);

    /* the size in bits of the variable value, e.g., 64 if value's type is int64_t */
    size_t size = sizeof(int64_t) * 8;

    while (true) {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        /* the following is only necessary if the implementation of >>= uses a
            logical shift rather than an arithmetic shift for a signed left operand */
        if (negative)
            value |= (~0 << (size - 7)); /* sign extend */

        /* sign bit of byte is second high-order bit (0x40) */
        if (value == 0 && !(value & 0x40)) {
            vm_serialize_binary_buf_put(buf, byte);
            break;
        }
        if (value == -1 && (value & 0x40)) {
            vm_serialize_binary_buf_put(buf, byte);
            break;
        }
        vm_serialize_binary_buf_put(buf, byte | 0x80);
    }

}

void vm_serialize_binary_buf_value(vm_serialize_binary_buf_t *buf, vm_std_value_t value) {
    vm_serialize_binary_buf_put(buf, value.tag);
    switch (value.tag) {
    case VM_TAG_I8: {
        vm_serialize_binary_buf_leb128(buf, value.value.i8);
        break;
    }
    case VM_TAG_I16: {
        vm_serialize_binary_buf_leb128(buf, value.value.i16);
        break;
    }
    case VM_TAG_I32: {
        vm_serialize_binary_buf_leb128(buf, value.value.i32);
        break;
    }
    case VM_TAG_I64: {
        vm_serialize_binary_buf_leb128(buf, value.value.i64);
        break;
    }
    case VM_TAG_F32: {
        vm_serialize_binary_buf_putn(buf, 4, &value.value.f32);
        break;
    }
    case VM_TAG_F64: {
        vm_serialize_binary_buf_putn(buf, 8, &value.value.f64);
        break;
    }
    }
}

vm_serialize_binary_buf_t vm_serialize_binary_value(vm_std_value_t value) {
    vm_serialize_binary_buf_t ret = {0};
    vm_serialize_binary_buf_value(&ret, value);
    return ret;
}

vm_table_t *vm_serialize_binary_value_table(vm_std_value_t value) {
    vm_serialize_binary_buf_t buf = vm_serialize_binary_value(value);
    vm_table_t *ret = vm_table_new();
    for (size_t i = 0; i < buf.len; i++) {
        vm_table_set(ret, (vm_value_t) {.i32 = i + 1}, (vm_value_t) {.i8 = buf.mem[i]}, VM_TAG_I32, VM_TAG_I8);
    }
    return ret;
}
