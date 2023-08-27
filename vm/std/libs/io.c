
#include "./io.h"
#include "../util.h"

vm_std_value_t vm_std_io_putchar(vm_std_value_t *args) {
    double num = 0;
    if (vm_std_parse_args(args, "f", &num)) {
        printf("%c", (int)num);
        return (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
    }
    ptrdiff_t inum = 0;
    if (vm_std_parse_args(args, "i", &inum)) {
        printf("%c", (int)inum);
        return (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
    }
    fprintf(stderr, "std.io.putchar: expected an int or float");
    exit(1);
}

char *vm_io_read(const char *filename) {
    void *file = fopen(filename, "rb");
    if (file == NULL) {
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

static void vm_indent(FILE *out, size_t indent, const char *prefix) {
    while (indent-- > 0) {
        fprintf(out, "    ");
    }
    fprintf(out, "%s", prefix);
}

void vm_io_debug(FILE *out, size_t indent, const char *prefix, vm_std_value_t value, vm_io_debug_t *link) {
    size_t up = 1;
    while (link != NULL) {
        if (value.tag == link->value.tag) {
            if (value.value.all == link->value.value.all) {
                vm_indent(out, indent, prefix);
                fprintf(out, "<ref %zu>\n", up);
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
    switch (value.tag) {
        case VM_TAG_NIL: {
            vm_indent(out, indent, prefix);
            fprintf(out, "nil\n");
            break;
        }
        case VM_TAG_BOOL: {
            vm_indent(out, indent, prefix);
            if (value.value.b) {
                fprintf(out, "true\n");
            } else {
                fprintf(out, "false\n");
            }
            break;
        }
        case VM_TAG_I64: {
            vm_indent(out, indent, prefix);
            fprintf(out, "%" PRIi64 "\n", value.value.i64);
            break;
        }
        case VM_TAG_F64: {
            vm_indent(out, indent, prefix);
            fprintf(out, "%f\n", value.value.f64);
            break;
        }
        case VM_TAG_STR: {
            vm_indent(out, indent, prefix);
            fprintf(out, "\"%s\"\n", value.value.str);
            break;
        }
        case VM_TAG_FUN: {
            vm_indent(out, indent, prefix);
            fprintf(out, "<function: %p>\n", value.value.all);
            break;
        }
        case VM_TAG_TAB: {
            vm_table_t *tab = value.value.table;
            vm_indent(out, indent, prefix);
            fprintf(out, "table {\n");
            for (size_t i = 0; i * sizeof(vm_pair_t) < tab->nbytes; i++) {
                vm_pair_t p = tab->pairs[i];
                switch (p.key_tag) {
                    case VM_TAG_NIL: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        vm_io_debug(out, indent + 1, "nil = ", val, &next);
                        break;
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
                        snprintf(buf, 63, "%f = ", p.key_val.f64);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    case VM_TAG_STR: {
                        vm_std_value_t val = (vm_std_value_t){
                            .tag = p.val_tag,
                            .value = p.val_val,
                        };
                        char buf[64];
                        snprintf(buf, 63, "%s = ", p.key_val.str);
                        vm_io_debug(out, indent + 1, buf, val, &next);
                        break;
                    }
                    default: {
                        vm_indent(out, indent + 1, "");
                        fprintf(out, "pair {\n");
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
                        fprintf(out, "}\n");
                    }
                }
            }
            vm_indent(out, indent, "");
            fprintf(out, "}\n");
            break;
        }
        case VM_TAG_FFI: {
            vm_indent(out, indent, prefix);
            fprintf(out, "<function: %p>\n", value.value.all);
            break;
        }
        default: {
            __builtin_trap();
            fprintf(out, "<T%zu: %p>\n", (size_t)value.tag, value.value.all);
            break;
        }
    }
}

vm_std_value_t vm_std_io_debug(vm_std_value_t *args) {
    while (args->tag != 0) {
        vm_io_debug(stdout, 0, "", *args++, NULL);
    }
    return (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}
