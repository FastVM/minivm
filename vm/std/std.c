
#include "./std.h"

#include "./libs/io.h"
#include "./util.h"

void vm_std_os_clock(vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    *ret = (vm_std_value_t){
        .tag = VM_TAG_F64,
        .value.f64 = (double)clock() / CLOCKS_PER_SEC,
    };
}

void vm_std_print(vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    FILE *out = stdout;
    bool first = true;
    while (args->tag != 0) {
        if (!first) {
            fprintf(out, "\t");
        }
        vm_std_value_t value = *args++;
        switch (value.tag) {
            case VM_TAG_NIL: {
                fprintf(out, "nil");
                break;
            }
            case VM_TAG_BOOL: {
                fprintf(out, "%s", value.value.b ? "true" : "false");
                break;
            }
            case VM_TAG_I8: {
                fprintf(out, "%" PRIi8, value.value.i8);
                break;
            }
            case VM_TAG_I16: {
                fprintf(out, "%" PRIi16, value.value.i16);
                break;
            }
            case VM_TAG_I32: {
                fprintf(out, "%" PRIi32, value.value.i32);
                break;
            }
            case VM_TAG_I64: {
                fprintf(out, "%" PRIi64, value.value.i64);
                break;
            }
            case VM_TAG_F32: {
                fprintf(out, "%f", value.value.f32);
                break;
            }
            case VM_TAG_F64: {
                fprintf(out, "%f", value.value.f64);
                break;
            }
            case VM_TAG_STR: {
                fprintf(out, "%s", value.value.str);
                break;
            }
            case VM_TAG_FUN: {
                fprintf(out, "<function: %p>", value.value.all);
                break;
            }
            case VM_TAG_TAB: {
                fprintf(out, "<table: %p>", value.value.table);
                break;
            }
            case VM_TAG_FFI: {
                fprintf(out, "<function: %p>", value.value.all);
                break;
            }
        }
        first = false;
    }
    fprintf(out, "\n");
    *ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

vm_table_t *vm_std_new(void) {
    vm_table_t *std = vm_table_new();

    vm_table_t *io = vm_table_new();
    VM_STD_SET_TAB(std, "io", io);

    vm_table_t *os = vm_table_new();
    VM_STD_SET_TAB(std, "os", os);
    VM_STD_SET_FFI(os, "clock", &vm_std_os_clock);

    VM_STD_SET_FFI(std, "print", &vm_std_print);

    return std;
}
