
#if !defined(VM_HEADER_STD_LIBS_IO)
#define VM_HEADER_STD_LIBS_IO

struct vm_io_debug_t;
typedef struct vm_io_debug_t vm_io_debug_t;

#include "obj.h"

struct vm_io_debug_t {
    vm_io_debug_t *next;
    vm_obj_t value;
};

vm_io_buffer_t *vm_io_buffer_new(void);
vm_io_buffer_t *vm_io_buffer_from_str(const char *str);
char *vm_io_buffer_get(vm_io_buffer_t *buf);

char *vm_io_format(const char *fmt, ...);
char *vm_io_read(const char *filename);
char *vm_io_vformat(const char *fmt, va_list ap);
void vm_io_buffer_format(vm_io_buffer_t *buf, const char *fmt, ...);
void vm_io_buffer_obj_debug(vm_io_buffer_t *out, size_t indent, const char *prefix, vm_obj_t value, vm_io_debug_t *link);
void vm_io_buffer_object_tostring(vm_io_buffer_t *buf, vm_obj_t value);
void vm_io_buffer_print_lit(vm_io_buffer_t *out, vm_obj_t value);
void vm_io_buffer_vformat(vm_io_buffer_t *buf, const char *fmt, va_list ap);

#endif
