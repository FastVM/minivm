
#include "./std.h"
#include "./util.h"

#include "./libs/io.h"
#include "./libs/os.h"
#include "./libs/dot.h"

vm_table_t *vm_std_new(void) {
    vm_table_t *std = vm_table_new();
    
    vm_table_t *io = vm_table_new();
    VM_STD_SET_FFI(io, "debug", &vm_std_io_debug);
    VM_STD_SET_FFI(io, "putchar", &vm_std_io_putchar);
    VM_STD_SET_TAB(std, "io", io);
    
    vm_table_t *os = vm_table_new();
    VM_STD_SET_FFI(os, "system", &vm_std_os_system);
    VM_STD_SET_FFI(os, "import", &vm_std_os_import);
    VM_STD_SET_TAB(std, "os", os);

    vm_table_t *dot = vm_table_new();
    VM_STD_SET_TAB(std, "dot", dot);
    VM_STD_SET_FFI(dot, "parse", &vm_std_dot_parse);
    VM_STD_SET_FFI(dot, "file", &vm_std_dot_file);

    VM_STD_SET_FFI(std, "eval", &vm_std_os_eval);

    return std;
}
