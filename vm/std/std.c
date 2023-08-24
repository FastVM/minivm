
#include "./std.h"

#include "./util.h"

vm_std_value_t vm_std_extern(vm_std_value_t *args) {
    const char *str;
    if (vm_std_parse_args(args, "s", &str)) {
        static void *handle = NULL;
        if (handle == NULL) {
            handle = dlopen(NULL, RTLD_LAZY);
        }
        void *sym = dlsym(handle, str);
        if (sym == NULL) {
            fprintf(stderr, "error: std.extern: unknown symbol: %s", str);
            exit(1);
        }
        return (vm_std_value_t){
            .value = (vm_value_t){
                .all = sym,
            },
            .tag = VM_TAG_FFI,
        };
    }
    fprintf(stderr, "error: std.extern: expected a string");
    exit(1);
}

vm_table_t *vm_std_new(void) {
    vm_table_t *ret = vm_table_new();
    vm_table_set(
        ret,
        (vm_value_t){
            .str = "extern",
        },
        (vm_value_t){
            .all = &vm_std_extern,
        },
        VM_TAG_STR,
        VM_TAG_FFI);
    return ret;
}
