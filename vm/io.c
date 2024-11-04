
#include "io.h"
#include "math.h"
#include "lib.h"
#include "obj.h"
#include "tables.h"

#include <stdio.h>

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
    vm_io_buffer_t buf;
    memset(&buf, 0, sizeof(vm_io_buffer_t));
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

static void vm_io_indent(vm_io_buffer_t *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        vm_io_buffer_format(out, "    ");
    }
    vm_io_buffer_format(out, "%s", prefix);
}

void vm_io_buffer_print_lit(vm_io_buffer_t *out, vm_obj_t value) {
    if (vm_obj_is_nil(value)) {
            vm_io_buffer_format(out, "nil");
    } else if (vm_obj_is_boolean(value)) {
        vm_io_buffer_format(out, "%s", vm_obj_get_boolean(value) ? "true" : "false");
    } else if (vm_obj_is_number(value)) {
        vm_io_buffer_format(out, VM_FORMAT_FLOAT, vm_obj_get_number(value));
    } else if (vm_obj_is_ffi(value)) {
        vm_io_buffer_format(out, "<function: %p>", vm_obj_get_ffi(value));
    } else if (vm_obj_is_buffer(value)) {
        vm_io_buffer_format(out, "\"%s\"", vm_obj_get_buffer(value)->buf);
    } else if (vm_obj_is_table(value)) {
        vm_io_buffer_format(out, "<table: %p>", vm_obj_get_table(value));
    } else if (vm_obj_is_closure(value)) {
        vm_io_buffer_format(out, "<closure: %p>", vm_obj_get_closure(value));
    } else {
        vm_io_buffer_format(out, "<object>");
    }
}

void vm_io_buffer_obj_debug(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_obj_t value, vm_io_debug_t *link) {
    size_t up = 1;
    while (link != NULL) {
        if (vm_obj_unsafe_eq(value, link->value)) {
            vm_io_indent(out, indent, prefix);
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
    if (vm_obj_is_nil(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, "nil\n");
    }
    if (vm_obj_is_boolean(value)) {
        vm_io_indent(out, indent, prefix);
        if (vm_obj_get_boolean(value)) {
            vm_io_buffer_format(out, "true\n");
        } else {
            vm_io_buffer_format(out, "false\n");
        }
    }
    if (vm_obj_is_number(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, VM_FORMAT_FLOAT "\n", vm_obj_get_number(value));
    }
    if (vm_obj_is_buffer(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, "\"%s\"\n", vm_obj_get_buffer(value)->buf);
    }
    if (vm_obj_is_closure(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, "<function: %p>\n", vm_obj_get_closure(value));
    }
    if (vm_obj_is_block(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, "<code: %p>\n", vm_obj_get_block(value));
    }
    if (vm_obj_is_table(value)) {
        vm_obj_table_t *tab = vm_obj_get_table(value);
        if (tab == NULL) {
            vm_io_indent(out, indent, prefix);
            vm_io_buffer_format(out, "table(NULL)\n");
        } else {
            vm_io_indent(out, indent, prefix);
            vm_io_buffer_format(out, "table(%p) {\n", tab);
            size_t len = vm_primes_table[tab->size];
            for (size_t i = 0; i < len; i++) {
                vm_obj_t key = tab->entries[i];
                if (vm_obj_is_nil(key)) {
                    // no print for empty keys
                } else if (vm_obj_is_boolean(key)) {
                    if (vm_obj_get_boolean(value)) {
                        vm_io_buffer_obj_debug(out, indent + 1, "[[true]] = ", tab->entries[vm_primes_table[tab->size] + i], &next);
                    } else {
                        vm_io_buffer_obj_debug(out, indent + 1, "[[false]] = ", tab->entries[vm_primes_table[tab->size] + i], &next);
                    }
                } else if (vm_obj_is_number(key)) {
                    char buf[64];
                    snprintf(buf, 63, "[[" VM_FORMAT_FLOAT "]] = ", vm_obj_get_number(key));
                    vm_io_buffer_obj_debug(out, indent + 1, buf, tab->entries[vm_primes_table[tab->size] + i], &next);
                }
                else if (vm_obj_is_buffer(key)) {
                    vm_io_buffer_t *buf = vm_io_buffer_new();
                    vm_io_buffer_format(buf, "%s = ", vm_obj_get_buffer(key)->buf);
                    vm_io_buffer_obj_debug(out, indent + 1, buf->buf, tab->entries[vm_primes_table[tab->size] + i], &next);
                    vm_free(buf->buf);
                    vm_free(buf);
                } else {
                    vm_io_indent(out, indent + 1, "");
                    vm_io_buffer_format(out, "pair {\n");
                    vm_io_buffer_obj_debug(out, indent + 2, "key = ", key, &next);
                    vm_io_buffer_obj_debug(out, indent + 2, "val = ", tab->entries[vm_primes_table[tab->size] + i], &next);
                    vm_io_indent(out, indent + 1, "");
                    vm_io_buffer_format(out, "}\n");
                }
            }
        }
        vm_io_indent(out, indent, "");
        vm_io_buffer_format(out, "}\n");
    }
    if (vm_obj_is_ffi(value)) {
        vm_io_indent(out, indent, prefix);
        vm_io_buffer_format(out, "<function: %p>\n", vm_obj_get_ffi(value));
    }
}

void vm_io_buffer_object_tostring(vm_io_buffer_t *buf, vm_obj_t value) {
    if (vm_obj_is_nil(value)) {
        vm_io_buffer_format(buf, "nil");
    }
    if (vm_obj_is_boolean(value)) {
        vm_io_buffer_format(buf, "%s", vm_obj_get_boolean(value) ? "true" : "false");
    }
    if (vm_obj_is_number(value)) {
        vm_io_buffer_format(buf, VM_FORMAT_FLOAT, vm_obj_get_number(value));
    }
    if (vm_obj_is_buffer(value)) {
        vm_io_buffer_format(buf, "%s", vm_obj_get_buffer(value)->buf);
    }
    if (vm_obj_is_closure(value)) {
        vm_io_buffer_format(buf, "<function: %p>", vm_obj_get_closure(value));
    }
    if (vm_obj_is_block(value)) {
        vm_io_buffer_format(buf, "<code: %p>", vm_obj_get_block(value));
    }
    if (vm_obj_is_table(value)) {
        vm_io_buffer_format(buf, "<table: %p>", vm_obj_get_table(value));
    }
    if (vm_obj_is_ffi(value)) {
        vm_io_buffer_format(buf, "<function: %p>", vm_obj_get_ffi(value));
    }
}

vm_io_buffer_t *vm_io_buffer_from_str(const char *str) {
    vm_io_buffer_t *buf = vm_io_buffer_new();
    vm_io_buffer_format(buf, "%s", str);
    return buf;
}
