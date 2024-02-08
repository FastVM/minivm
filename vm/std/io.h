
#if !defined(VM_HEADER_STD_LIBS_IO)
#define VM_HEADER_STD_LIBS_IO

struct vm_io_debug_t;
typedef struct vm_io_debug_t vm_io_debug_t;

struct vm_io_buffer_t;
typedef struct vm_io_buffer_t vm_io_buffer_t;

#include "../obj.h"

struct vm_io_debug_t {
    vm_io_debug_t *next;
    vm_std_value_t value;
};

struct vm_io_buffer_t {
    size_t len;
    char *buf;
    size_t alloc;
};

vm_io_buffer_t *vm_io_buffer_new(void);
char *vm_io_buffer_get(vm_io_buffer_t *buf);
void vm_io_print_lit(vm_io_buffer_t *out, vm_std_value_t value);
void vm_io_debug(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_std_value_t value, vm_io_debug_t *link);
char *vm_io_read(const char *filename);
void vm_io_buffer_vformat(vm_io_buffer_t *buf, const char *fmt, va_list ap);
void vm_io_buffer_format(vm_io_buffer_t *buf, const char *fmt, ...);
char *vm_io_vformat(const char *fmt, va_list ap);
char *vm_io_format(const char *fmt, ...);
void vm_value_buffer_tostring(vm_io_buffer_t *buf, vm_std_value_t value);

#endif
