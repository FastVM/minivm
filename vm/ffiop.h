#include <vm/nanbox.h>
#include <vm/gc.h>

enum vm_ffi_type;
enum vm_ffi_error_t;
struct vm_ffi_res_t;

typedef enum vm_ffi_type vm_ffi_type;
typedef enum vm_ffi_error_t vm_ffi_error_t;
typedef struct vm_ffi_res_t vm_ffi_res_t;

enum vm_ffi_type
{
    VM_FFI_TYPE_VOID,
    VM_FFI_TYPE_BOOL,
    VM_FFI_TYPE_CHAR,
    VM_FFI_TYPE_SCHAR,
    VM_FFI_TYPE_UCHAR,
    VM_FFI_TYPE_INT8,
    VM_FFI_TYPE_UINT8,
    VM_FFI_TYPE_INT16,
    VM_FFI_TYPE_UINT16,
    VM_FFI_TYPE_INT32,
    VM_FFI_TYPE_UINT32,
    VM_FFI_TYPE_INT64,
    VM_FFI_TYPE_FLOAT32,
    VM_FFI_TYPE_FLOAT64,
    VM_FFI_TYPE_UINT64,
    VM_FFI_TYPE_STRING,
};

enum vm_ffi_error_t
{
    VM_FFI_NO_ERROR,
    VM_FFI_ERROR_OPENING,
    VM_FFI_ERROR_SYMBOL,
    VM_FFI_ERROR_CIF,
    VM_FFI_ERROR_OTHER,
};

struct vm_ffi_res_t
{
    vm_ffi_error_t state;
    nanbox_t result;
};

vm_ffi_res_t vm_ffi_opcode(vm_gc_t*gc, nanbox_t library, nanbox_t function, nanbox_t retty, nanbox_t argty, nanbox_t arguments);