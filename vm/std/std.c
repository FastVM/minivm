
#include "./std.h"
#include "./util.h"

#include "./libs/io.h"

vm_table_t *vm_std_new(void) {
    vm_table_t *std = vm_table_new();
    
    vm_table_t *io = vm_table_new();
    VM_STD_SET_FFI(io, "debug", &vm_std_io_debug);
    VM_STD_SET_FFI(io, "putchar", &vm_std_io_putchar);
    
    VM_STD_SET_TAB(std, "io", io);

    return std;
}
