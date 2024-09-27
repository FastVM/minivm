#if !defined(VM_HEADER_STD_STD)
#define VM_HEADER_STD_STD

#include "io.h"

#define VM_STD_REF(vm, x) (&(x))

void vm_io_buffer_object_tostring(vm_io_buffer_t *buf, vm_obj_t value);
void vm_std_set_arg(vm_t *vm, const char *prog, const char *file, int argc, char **argv);
void vm_std_new(vm_t *vm);

#endif
