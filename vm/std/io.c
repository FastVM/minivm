
#include "./io.h"

void vm_io_buffer_vformat(vm_io_buffer_t *buf, const char *fmt, va_list ap) {
    while (true) {
        int avail = buf->alloc - buf->len;
        va_list ap_copy;
        va_copy(ap_copy, ap);
        int written = vsnprintf(&buf->buf[buf->len], avail, fmt, ap_copy);
        va_end(ap_copy);
        if (avail <= written) {
            buf->alloc = buf->alloc * 2 + 16;
            buf->buf = vm_realloc(buf->buf, sizeof(char) * buf->alloc);
            continue;
        }
        buf->len += written;
        break;
    }
}

char *vm_io_buffer_get(vm_io_buffer_t *buf) {
    return buf->buf;
}

vm_io_buffer_t *vm_io_buffer_new(void) {
    vm_io_buffer_t *ret = vm_malloc(sizeof(vm_io_buffer_t));
    char *buf = vm_malloc(sizeof(char));
    *buf = '\0';
    *ret = (vm_io_buffer_t) {
        .alloc = 1,
        .buf = buf,
        .len = 0,
    };
    return ret;
}

void vm_io_buffer_format(vm_io_buffer_t *buf, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vm_io_buffer_vformat(buf, fmt, ap);
    va_end(ap);
}

char *vm_io_vformat(const char *fmt, va_list ap) {
    vm_io_buffer_t buf = (vm_io_buffer_t) {0};
    vm_io_buffer_vformat(&buf, fmt, ap);
    return buf.buf;
}

char *vm_io_format(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *r = vm_io_vformat(fmt, ap);
    va_end(ap);
    return r;
}

char *vm_io_read(const char *vm_io_buffer_tname) {
    void *vm_io_buffer_t = fopen(vm_io_buffer_tname, "rb");
    if (vm_io_buffer_t == NULL) {
        return NULL;
    }
    size_t nalloc = 512;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    for (;;) {
        if (nops + 256 >= nalloc) {
            nalloc = (nops + 256) * 2;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
        size = fread(&ops[nops], 1, 256, vm_io_buffer_t);
        nops += size;
        if (size < 256) {
            break;
        }
    }
    ops[nops] = '\0';
    fclose(vm_io_buffer_t);
    return ops;
}

static void vm_indent(vm_io_buffer_t *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        vm_io_buffer_format(out, "    ");
    }
    vm_io_buffer_format(out, "%s", prefix);
}

void vm_io_print_lit(vm_io_buffer_t *out, vm_std_value_t value) {
    switch (value.tag.tag) {
        case VM_TAG_NIL: {
            vm_io_buffer_format(out, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            vm_io_buffer_format(out, "%s", value.value.b ? "true" : "false");
            break;
        }
        case VM_TAG_I8: {
            vm_io_buffer_format(out, "%" PRIi8, value.value.i8);
            break;
        }
        case VM_TAG_I16: {
            vm_io_buffer_format(out, "%" PRIi16, value.value.i16);
            break;
        }
        case VM_TAG_I32: {
            vm_io_buffer_format(out, "%" PRIi32, value.value.i32);
            break;
        }
        case VM_TAG_I64: {
            vm_io_buffer_format(out, "%" PRIi64, value.value.i64);
            break;
        }
        case VM_TAG_F32: {
            vm_io_buffer_format(out, VM_FORMAT_FLOAT, value.value.f32);
            break;
        }
        case VM_TAG_F64: {
            vm_io_buffer_format(out, VM_FORMAT_FLOAT, value.value.f64);
            break;
        }
        case VM_TAG_FFI: {
            vm_io_buffer_format(out, "<function: %p>", value.value.ffi);
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_format(out, "\"%s\"", value.value.str);
            break;
        }
    }
}

void vm_io_debug(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_std_value_t value, vm_io_debug_t *link) {
    size_t up = 1;
    while (link != NULL) {
        if (vm_type_eq(value.tag, link->value.tag)) {
            if (value.value.all == link->value.value.all) {
                vm_indent(out, indent, prefix);
                vm_io_buffer_format(out, "<ref %zu>\n", up);
                return;
            }
        }
        up += 1;
        link = link->next;
    }
    vm_io_debug_t next = (vm_io_debug_t){
        .next = link,
        .value = value,
    };
    switch (value.tag.tag) {
        case VM_TAG_NIL: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "nil\n");
            break;
        }
        case VM_TAG_BOOL: {
            vm_indent(out, indent, prefix);
            if (value.value.b) {
                vm_io_buffer_format(out, "true\n");
            } else {
                vm_io_buffer_format(out, "false\n");
            }
            break;
        }
        case VM_TAG_I8: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "%" PRIi8 "\n", value.value.i8);
            break;
        }
        case VM_TAG_I16: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "%" PRIi16 "\n", value.value.i16);
            break;
        }
        case VM_TAG_I32: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "%" PRIi32 "\n", value.value.i32);
            break;
        }
        case VM_TAG_I64: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "%" PRIi64 "\n", value.value.i64);
            break;
        }
        case VM_TAG_F32: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, VM_FORMAT_FLOAT "\n", value.value.f32);
            break;
        }
        case VM_TAG_F64: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, VM_FORMAT_FLOAT "\n", value.value.f64);
            break;
        }
        case VM_TAG_STR: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "\"%s\"\n", value.value.str);
            break;
        }
        case VM_TAG_CLOSURE: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "<function: %p>\n", value.value.all);
            break;
        }
        case VM_TAG_FUN: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "<code: %p>\n", value.value.all);
            break;
        }
        case VM_TAG_TAB: {
            vm_table_t *tab = value.value.table;
            if (tab == NULL) {
                vm_indent(out, indent, prefix);
                vm_io_buffer_format(out, "table(NULL)\n");
                break;
            }
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "table(%p) {\n", tab);
            size_t len = 1 << tab->alloc;
            for (size_t i = 0; i < len; i++) {
                vm_pair_t p = tab->pairs[i];
                switch (p.key_tag.tag) {
                    case 0: {
                        break;
                    }
                    case VM_TAG_NIL: {
                        __builtin_trap();
                    }
                    case VM_TAG_BOOL: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        if (value.value.b) {
                            vm_io_debug(out, indent + 1, "true = ", val, &next);
                        } else {
                            vm_io_debug(out, indent + 1, "false = ", val, &next);
                        }
                        break;
                    }
                    // case VM_TAG_I64: {
                    //     vm_std_value_t val = (vm_std_value_t){
                    //         .tag = p.val_tag,
                    //         .value = p.val_val,
                    //     };
                    //     char buf[64];
                    //     snprintf(buf, 63, "%" PRIi64 " = ", p.key_val.i64);
                    //     vm_io_debug(out, indent + 1, buf, val, &next);
                    //     break;
                    // }
                    case VM_TAG_I8: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, "%" PRIi8 " = ", p.key_val.i8);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_I16: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, "%" PRIi16 " = ", p.key_val.i16);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_I32: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, "%" PRIi32 " = ", p.key_val.i32);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_I64: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, "%" PRIi64 " = ", p.key_val.i64);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_F64: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, VM_FORMAT_FLOAT " = ", p.key_val.f64);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_STR: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        vm_io_buffer_t buf = {0};
                        vm_io_buffer_format(&buf, "%s = ", p.key_val.str);
                        vm_io_debug(out, indent + 1, buf.buf, val, &next);
                        vm_free(buf.buf);
                        break;
                    }
                    default: {
                        vm_indent(out, indent + 1, "");
                        vm_io_buffer_format(out, "pair {\n");
                        vm_std_value_t key = (vm_std_value_t){
                            .tag = p.key_tag,
                            .value = p.key_val,
                        };
                        vm_io_debug(out, indent + 2, "key = ", key, &next);
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        vm_io_debug(out, indent + 2, "val = ", val, &next);
                        vm_indent(out, indent + 1, "");
                        vm_io_buffer_format(out, "}\n");
                    }
                }
            }
            vm_indent(out, indent, "");
            vm_io_buffer_format(out, "}\n");
            break;
        }
        case VM_TAG_FFI: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "<function: %p>\n", value.value.all);
            break;
        }
        default: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "<0x%zx: %p>\n", (size_t)value.tag.tag, value.value.all);
            // __builtin_trap();
            break;
        }
    }
}

void vm_value_buffer_tostring(vm_io_buffer_t *buf, vm_std_value_t value) {
    switch (value.tag.tag) {
        case VM_TAG_NIL: {
            vm_io_buffer_format(buf, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            vm_io_buffer_format(buf, "%s", value.value.b ? "true" : "false");
            break;
        }
        case VM_TAG_I8: {
            vm_io_buffer_format(buf, "%" PRIi8, value.value.i8);
            break;
        }
        case VM_TAG_I16: {
            vm_io_buffer_format(buf, "%" PRIi16, value.value.i16);
            break;
        }
        case VM_TAG_I32: {
            vm_io_buffer_format(buf, "%" PRIi32, value.value.i32);
            break;
        }
        case VM_TAG_I64: {
            vm_io_buffer_format(buf, "%" PRIi64, value.value.i64);
            break;
        }
        case VM_TAG_F32: {
            vm_io_buffer_format(buf, VM_FORMAT_FLOAT, value.value.f32);
            break;
        }
        case VM_TAG_F64: {
            vm_io_buffer_format(buf, VM_FORMAT_FLOAT, value.value.f64);
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_format(buf, "%s", value.value.str);
            break;
        }
        case VM_TAG_CLOSURE: {
            vm_io_buffer_format(buf, "<function: %p>", value.value.closure);
            break;
        }
        case VM_TAG_FUN: {
            vm_io_buffer_format(buf, "<code: %p>", value.value.all);
            break;
        }
        case VM_TAG_TAB: {
            vm_io_buffer_format(buf, "<table: %p>", value.value.table);
            break;
        }
        case VM_TAG_FFI: {
            vm_io_buffer_format(buf, "<function: %p>", value.value.all);
            break;
        }
    }
}
