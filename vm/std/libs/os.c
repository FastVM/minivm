
#include "./os.h"
#include "../util.h"
#include "./io.h"
#include "../../ir.h"
#include "../../jit/x64.h"
#include "../../lang/paka.h"

vm_std_value_t vm_std_os_system(vm_std_value_t *args) {
    const char *str = NULL;
    if (vm_std_parse_args(args, "s", &str)) {
        return (vm_std_value_t){
            .tag = VM_TAG_F64,
            .value.f64 = (double)system(str),
        };
    }
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}

vm_std_value_t vm_std_os_import(vm_std_value_t *args) {
    const char *name = NULL;
    if (vm_std_parse_args(args, "s", &name)) {
        char *src = vm_io_read(name);
        if (src == NULL) {
            fprintf(stderr, "could not read file\n");
            goto fail;
        }
        vm_block_t *block = vm_paka_parse(src);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            goto fail;
        }
        vm_std_value_t sub_args[1];
        sub_args[0].tag = VM_TAG_UNK;
        vm_table_t *std = vm_std_new();
        return vm_x64_run(block, std, sub_args);
    }
fail:;
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}


vm_std_value_t vm_std_os_eval(vm_std_value_t *args) {
    const char *name = NULL;
    if (vm_std_parse_args(args, "s", &name)) {
        vm_block_t *block = vm_paka_parse(name);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            goto fail;
        }
        vm_std_value_t sub_args[1];
        sub_args[0].tag = VM_TAG_UNK;
        vm_table_t *std = vm_std_new();
        return vm_x64_run(block, std, sub_args);
    }
fail:;
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}
