
#include "./std.h"

#include "./io.h"
#include "./util.h"
#include "../ast/ast.h"

vm_ast_node_t vm_lang_lua_parse(vm_config_t *config, const char *str);
void vm_ast_comp_more(vm_ast_node_t node, vm_blocks_t *blocks);

void vm_std_os_exit(vm_std_closure_t *closure, vm_std_value_t *args) {
    exit((int)vm_value_to_i64(args[0]));
}

void vm_std_load(vm_std_closure_t *closure, vm_std_value_t *args) {
    if (args[0].tag != VM_TAG_STR) {
        *args = (vm_std_value_t) {
            .tag = VM_TAG_ERROR,
            .value.str = "cannot load non-string value",
        };
    }
    const char *str = args[0].value.str;
    vm_ast_node_t node = vm_lang_lua_parse(closure->config, str);
    vm_ast_comp_more(node, closure->blocks);
    vm_ast_free_node(node);
    
    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * 2);
    vals[0] = (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = 1,
    };
    vals += 1;
    vals[0] = (vm_std_value_t) {
        .tag = VM_TAG_FUN,
        .value.i32 = (int32_t) closure->blocks->entry->id,
    };
    // vm_io_buffer_t buf = {0};
    // vm_io_format_blocks(&buf, closure->blocks);
    // printf("%s\n", buf.buf);
    *args = (vm_std_value_t) {
        .tag = VM_TAG_CLOSURE,
        .value.closure = vals,
    };
}

void vm_std_assert(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_std_value_t val = args[0];
    if (val.tag == VM_TAG_NIL || (val.tag == VM_TAG_BOOL && !val.value.b)) {
        vm_std_value_t msg = args[1];
        vm_io_buffer_t buf = {0};
        vm_io_print_lit(&buf, msg);
        *args = (vm_std_value_t){
            .tag = VM_TAG_ERROR,
            .value.str = buf.buf,
        };
    } else {
        *args = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
    }
}

void vm_std_vm_closure(vm_std_closure_t *closure, vm_std_value_t *args) {
    int64_t nargs = 0;
    for (size_t i = 0; args[i].tag != 0; i++) {
        nargs += 1;
    }
    if (nargs == 0 || args[0].tag != VM_TAG_FUN) {
        *args = (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
        return;
    }
    vm_std_value_t *vals = vm_malloc(sizeof(vm_std_value_t) * (nargs + 1));
    vals[0] = (vm_std_value_t) {
        .tag = VM_TAG_I32,
        .value.i32 = (int32_t) nargs,
    };
    vals += 1;
    for (size_t i = 0; args[i].tag != 0; i++) {
        vals[i] = args[i];
    }
    *args = (vm_std_value_t){
        .tag = VM_TAG_CLOSURE,
        .value.closure = vals,
    };
    return;
}

void vm_std_vm_print(vm_std_closure_t *closure, vm_std_value_t *args) {
    for (size_t i = 0; args[i].tag; i++) {
        vm_io_buffer_t buf = {0};
        vm_io_debug(&buf, 0, "", args[i], NULL);
        printf("%.*s", (int) buf.len, buf.buf);
    }
}

void vm_std_vm_concat(vm_std_closure_t *closure, vm_std_value_t *args) {
    size_t len = 1;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        len += strlen(args[i].value.str);
    }
    char *buf = vm_malloc(sizeof(char) * len);
    size_t head = 0;
    for (size_t i = 0; args[i].tag == VM_TAG_STR; i++) {
        size_t len = strlen(args[i].value.str);
        if (len == 0) {
            continue;
        }
        memcpy(&buf[head], args[i].value.str, len);
        head += len;
    }
    buf[len-1] = '\0';
    *args = (vm_std_value_t) {
        .tag = VM_TAG_STR,
        .value.str = buf,
    };
}

void vm_std_type(vm_std_closure_t *closure, vm_std_value_t *args) {
    const char *ret = "unknown";
    switch (args[0].tag) {
        case VM_TAG_NIL: {
            ret = "nil";
            break;
        }
        case VM_TAG_BOOL: {
            ret = "boolean";
            break;
        }
        case VM_TAG_I8: {
            ret = "number";
            break;
        }
        case VM_TAG_I16: {
            ret = "number";
            break;
        }
        case VM_TAG_I32: {
            ret = "number";
            break;
        }
        case VM_TAG_I64: {
            ret = "number";
            break;
        }
        case VM_TAG_F32: {
            ret = "number";
            break;
        }
        case VM_TAG_F64: {
            ret = "number";
            break;
        }
        case VM_TAG_STR: {
            ret = "string";
            break;
        }
        case VM_TAG_CLOSURE: {
            ret = "function";
            break;
        }
        case VM_TAG_FUN: {
            ret = "function";
            break;
        }
        case VM_TAG_TAB: {
            ret = "table";
            break;
        }
        case VM_TAG_FFI: {
            ret = "function";
            break;
        }
        case VM_TAG_ERROR: {
            ret = "string";
            break;
        }
    }
    *args = (vm_std_value_t) {
        .tag = VM_TAG_STR,
        .value.str = ret,
    };
}

void vm_std_tostring(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_io_buffer_t out = {0};
    vm_value_buffer_tostring(&out, *args);
    *args = (vm_std_value_t) {
        .tag = VM_TAG_STR,
        .value.str = out.buf,
    };
}

void vm_std_print(vm_std_closure_t *closure, vm_std_value_t *args) {
    vm_std_value_t *ret = args;
    vm_io_buffer_t out = {0};
    bool first = true;
    while (args->tag != 0) {
        if (!first) {
            vm_io_buffer_format(&out, "\t");
        }
        vm_value_buffer_tostring(&out, *args++);
        first = false;
    }
    fprintf(stdout, "%.*s\n", (int) out.len, out.buf);
    *ret = (vm_std_value_t){
        .tag = VM_TAG_NIL,
    };
}

vm_table_t *vm_std_new(void) {
    vm_table_t *std = vm_table_new();

    {
        vm_table_t *io = vm_table_new();
        VM_STD_SET_TAB(std, "io", io);
    }

    {
        vm_table_t *vm = vm_table_new();
        VM_STD_SET_FFI(vm, "print", &vm_std_vm_print);
        {
            vm_table_t *vm_ver = vm_table_new();
            VM_STD_SET_TAB(vm, "version", vm_ver);
        }
        VM_STD_SET_TAB(std, "vm", vm);
    }

    {
        vm_table_t *os = vm_table_new();
        VM_STD_SET_FFI(os, "exit", &vm_std_os_exit);
        VM_STD_SET_TAB(std, "os", os);
    }

    VM_STD_SET_FFI(std, "tostring", &vm_std_tostring);
    VM_STD_SET_FFI(std, "type", &vm_std_type);
    VM_STD_SET_FFI(std, "print", &vm_std_print);
    VM_STD_SET_FFI(std, "assert", &vm_std_assert);
    VM_STD_SET_FFI(std, "load", &vm_std_load);

    return std;
}
