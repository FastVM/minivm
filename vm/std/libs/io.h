
#if !defined(VM_HEADER_STD_LIBS_IO)
#define VM_HEADER_STD_LIBS_IO
#include "../std.h"

struct vm_io_debug_t;
typedef struct vm_io_debug_t vm_io_debug_t;

struct vm_io_debug_t {
    vm_io_debug_t *next;
    vm_std_value_t value;
};

void vm_io_print_num(FILE *out, vm_std_value_t value);
void vm_io_debug(FILE *out, size_t indent, const char *prefix, vm_std_value_t value, vm_io_debug_t *link);
char *vm_io_read(const char *filename);

#endif
