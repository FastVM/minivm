
#if !defined(VM_HEADER_FFI)
#define VM_HEADER_FFI

#include "./lib.h"
#include "./be/value.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(_POSIX_VERSION)
#include <dlfcn.h>
#else
#error bad platform
#endif

struct vm_ffi_handle_t;
typedef struct vm_ffi_handle_t vm_ffi_handle_t;

struct vm_ffi_symbol_t;
typedef struct vm_ffi_symbol_t vm_ffi_symbol_t;

struct vm_ffi_handle_t {
#if defined(_WIN32)
    HINSTANCE win; 
#elif defined(_POSIX_VERSION)
    void *pos;
#endif
};

struct vm_ffi_symbol_t {
    void (*func)(void);
    void *cif;
};

vm_ffi_handle_t *vm_ffi_handle_open(const char *filename);
void vm_ffi_handle_close(vm_ffi_handle_t *handle);

vm_ffi_symbol_t *vm_ffi_handle_get(vm_ffi_handle_t *handle, const char *str, vm_tag_t ret, size_t nargs, vm_tag_t *args);
vm_value_t vm_ffi_symbol_call(vm_ffi_symbol_t *sym, size_t nargs, vm_value_t *args);

#endif
