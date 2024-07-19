
#include "./io.h"

void vm_io_buffer_vformat(vm_io_buffer_t *buf, const char *fmt, va_list ap) {
    if (buf->buf == NULL) {
        buf->alloc = 16;
        buf->buf = vm_malloc(sizeof(char) * buf->alloc);
    }
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
    *ret = (vm_io_buffer_t){
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
    vm_io_buffer_t buf = (vm_io_buffer_t){0};
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

char *vm_io_read(const char *buf) {
    FILE *file = fopen(buf, "rb");
    if (file == NULL) {
        return NULL;
    }
    size_t nalloc = 512;
    char *ops = vm_malloc(sizeof(char) * nalloc);
    size_t nops = 0;
    size_t size;
    while (true) {
        if (nops + 256 >= nalloc) {
            nalloc = (nops + 256) * 2;
            ops = vm_realloc(ops, sizeof(char) * nalloc);
        }
        size = fread(&ops[nops], 1, 256, file);
        nops += size;
        if (size < 256) {
            break;
        }
    }
    ops[nops] = '\0';
    fclose(file);
    return ops;
}

static void vm_indent(vm_io_buffer_t *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        vm_io_buffer_format(out, "    ");
    }
    vm_io_buffer_format(out, "%s", prefix);
}

void vm_io_print_lit(vm_io_buffer_t *out, vm_obj_t value) {
    switch (value.tag) {
        case VM_TAG_NIL: {
            vm_io_buffer_format(out, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            vm_io_buffer_format(out, "%s", value.value.b ? "true" : "false");
            break;
        }
        case VM_TAG_NUMBER: {
            vm_io_buffer_format(out, VM_FORMAT_FLOAT, value.value.f64);
            break;
        }
        case VM_TAG_FFI: {
            vm_io_buffer_format(out, "<function: %p>", value.value.ffi);
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_format(out, "\"%s\"", value.value.str->buf);
            break;
        }
    }
}

void vm_io_debug(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_obj_t value, vm_io_debug_t *link) {
    size_t up = 1;
    while (link != NULL) {
        if (vm_obj_eq(value, link->value)) {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "<ref %zu>\n", up);
            return;
        }
        up += 1;
        link = link->next;
    }
    vm_io_debug_t next = (vm_io_debug_t){
        .next = link,
        .value = value,
    };
    switch (value.tag) {
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
        case VM_TAG_NUMBER: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, VM_FORMAT_FLOAT "\n", value.value.f64);
            break;
        }
        case VM_TAG_STR: {
            vm_indent(out, indent, prefix);
            vm_io_buffer_format(out, "\"%s\"\n", value.value.str->buf);
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
                vm_table_pair_t p = tab->pairs[i];
                switch (p.key.tag) {
                    case 0: {
                        break;
                    }
                    case VM_TAG_NIL: {
                        __builtin_trap();
                    }
                    case VM_TAG_BOOL: {
                        if (value.value.b) {
                            vm_io_debug(out, indent + 1, "true = ", p.value, &next);
                        } else {
                            vm_io_debug(out, indent + 1, "false = ", p.value, &next);
                        }
                        break;
                    }
                    case VM_TAG_NUMBER: {
                        char buf[64];
                        snprintf(buf, 63, VM_FORMAT_FLOAT " = ", p.key.value.f64);
                        vm_io_debug(out, indent + 1, buf, p.value, &next);
                        break;
                    }
                    case VM_TAG_STR: {
                        vm_io_buffer_t buf = {0};
                        vm_io_buffer_format(&buf, "%s = ", p.key.value.str->buf);
                        vm_io_debug(out, indent + 1, buf.buf, p.value, &next);
                        vm_free(buf.buf);
                        break;
                    }
                    default: {
                        vm_indent(out, indent + 1, "");
                        vm_io_buffer_format(out, "pair {\n");
                        vm_io_debug(out, indent + 2, "key = ", p.key, &next);
                        vm_io_debug(out, indent + 2, "val = ", p.value, &next);
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
            vm_io_buffer_format(out, "<0x%zx: %p>\n", (size_t)value.tag, value.value.all);
            break;
        }
    }
}

void vm_value_buffer_tostring(vm_io_buffer_t *buf, vm_obj_t value) {
    switch (value.tag) {
        case VM_TAG_NIL: {
            vm_io_buffer_format(buf, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            vm_io_buffer_format(buf, "%s", value.value.b ? "true" : "false");
            break;
        }
        case VM_TAG_NUMBER: {
            vm_io_buffer_format(buf, VM_FORMAT_FLOAT, value.value.f64);
            break;
        }
        case VM_TAG_STR: {
            vm_io_buffer_format(buf, "%s", value.value.str->buf);
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

vm_io_buffer_t *vm_io_buffer_from_str(const char *str) {
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_io_buffer_format(buf, "%s", str);
    return buf;
}
