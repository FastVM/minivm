#include <vm/ffiop.h>
#include <dlfcn.h>
#include <ffi.h>

void *vm_ffi_tostring(vm_gc_t *gc, nanbox_t ptr)
{
    int len = vm_gc_sizeof(gc, ptr);
    char *chrs = vm_mem_alloc((len + 1) * sizeof(char));
    for (int i = 0; i < len; i++)
    {
        nanbox_t chr = vm_gc_get(gc, ptr, i);
        chrs[i] = (char)nanbox_to_double(chr);
    }
    chrs[len] = '\0';
    return chrs;
}

nanbox_t vm_ffi_from_string(vm_gc_t *gc, const char *str)
{
    int len = strlen(str);
    nanbox_t ret = vm_gc_new(gc, len);
    for (int i = 0; i < len; i++) {
        nanbox_t chr = nanbox_from_double((double) (str[i]));
        vm_gc_set(gc, ret, i, chr);
    }
    return ret;
}

int vm_ffi_type_size(vm_gc_t *gc, nanbox_t type)
{

    if (nanbox_is_double(type))
    {
        vm_ffi_type repr = (int)nanbox_to_double(type);
        switch (repr)
        {
        case VM_FFI_TYPE_VOID:
            return 0;
        case VM_FFI_TYPE_BOOL:
            return sizeof(uint32_t);
        case VM_FFI_TYPE_INT8:
            return sizeof(int8_t);
        case VM_FFI_TYPE_INT16:
            return sizeof(int16_t);
        case VM_FFI_TYPE_INT32:
            return sizeof(int32_t);
        case VM_FFI_TYPE_INT64:
            return sizeof(int64_t);
        case VM_FFI_TYPE_UINT8:
            return sizeof(uint8_t);
        case VM_FFI_TYPE_UINT16:
            return sizeof(uint16_t);
        case VM_FFI_TYPE_UINT32:
            return sizeof(uint32_t);
        case VM_FFI_TYPE_UINT64:
            return sizeof(uint64_t);
        case VM_FFI_TYPE_FLOAT32:
            return sizeof(float);
        case VM_FFI_TYPE_FLOAT64:
            return sizeof(double);
        case VM_FFI_TYPE_STRING:
            return sizeof(char *);
        }
    }
    else
    {
        printf("internal error: type size: no generics yet\n");
        exit(0);
    }
}

ffi_type *vm_ffi_get_type(vm_gc_t *gc, nanbox_t type)
{
    if (nanbox_is_double(type))
    {
        vm_ffi_type repr = (int)nanbox_to_double(type);
        switch (repr)
        {
        case VM_FFI_TYPE_VOID:
            return &ffi_type_void;
        case VM_FFI_TYPE_BOOL:
            return &ffi_type_uint32;
        case VM_FFI_TYPE_INT8:
            return &ffi_type_sint8;
        case VM_FFI_TYPE_INT16:
            return &ffi_type_sint16;
        case VM_FFI_TYPE_INT32:
            return &ffi_type_sint32;
        case VM_FFI_TYPE_INT64:
            return &ffi_type_sint64;
        case VM_FFI_TYPE_UINT8:
            return &ffi_type_uint8;
        case VM_FFI_TYPE_UINT16:
            return &ffi_type_uint16;
        case VM_FFI_TYPE_UINT32:
            return &ffi_type_uint32;
        case VM_FFI_TYPE_UINT64:
            return &ffi_type_uint64;
        case VM_FFI_TYPE_FLOAT32:
            return &ffi_type_float;
        case VM_FFI_TYPE_FLOAT64:
            return &ffi_type_double;
        case VM_FFI_TYPE_STRING:
            return &ffi_type_pointer;
        default:
            printf("internal error: get type: unknown enum\n");
            exit(0);
        }
    }
    else
    {
        printf("internal error: get type: no generics yet\n");
        exit(0);
    }
}

void *vm_ffi_cast_input(vm_gc_t *gc, nanbox_t type, nanbox_t value)
{
    if (nanbox_is_double(type))
    {
        vm_ffi_type repr = (int)nanbox_to_double(type);
        switch (repr)
        {
        case VM_FFI_TYPE_VOID:
        {
            return NULL;
        }
        case VM_FFI_TYPE_BOOL:
        {
            uint32_t *ret = vm_mem_alloc(sizeof(uint32_t));
            *ret = (uint32_t)nanbox_to_boolean(value);
            return ret;
        }
        case VM_FFI_TYPE_INT8:
        {
            int8_t *ret = vm_mem_alloc(sizeof(int8_t));
            *ret = (int8_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_INT16:
        {
            int16_t *ret = vm_mem_alloc(sizeof(int16_t));
            *ret = (int16_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_INT32:
        {
            int32_t *ret = vm_mem_alloc(sizeof(int32_t));
            *ret = (int32_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_INT64:
        {
            int64_t *ret = vm_mem_alloc(sizeof(int64_t));
            *ret = (int64_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_UINT8:
        {
            uint8_t *ret = vm_mem_alloc(sizeof(uint8_t));
            *ret = (uint8_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_UINT16:
        {
            uint16_t *ret = vm_mem_alloc(sizeof(uint16_t));
            *ret = (uint16_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_UINT32:
        {
            uint32_t *ret = vm_mem_alloc(sizeof(uint32_t));
            *ret = (uint32_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_UINT64:
        {
            uint64_t *ret = vm_mem_alloc(sizeof(uint64_t));
            *ret = (uint64_t)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_FLOAT32:
        {
            float *ret = vm_mem_alloc(sizeof(float));
            *ret = (float)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_FLOAT64:
        {
            double *ret = vm_mem_alloc(sizeof(double));
            *ret = (double)nanbox_to_double(value);
            return ret;
        }
        case VM_FFI_TYPE_STRING:
        {
            char **ret = vm_mem_alloc(sizeof(char *));
            *ret = vm_ffi_tostring(gc, value);
            return ret;
        }
        default:
            printf("internal error: cast input: unknown enum\n");
            exit(0);
        }
    }
    else
    {
        printf("internal error: cast input: no generics yet\n");
        exit(0);
    }
}

nanbox_t vm_ffi_cast_output(vm_gc_t *gc, nanbox_t type, void *value)
{
    if (nanbox_is_double(type))
    {
        vm_ffi_type repr = (int)nanbox_to_double(type);
        switch (repr)
        {
        case VM_FFI_TYPE_VOID:
        {
            return nanbox_from_boolean(false);
        }
        case VM_FFI_TYPE_BOOL:
        {
            return nanbox_from_boolean((bool)*(uint32_t *)value);
        }
        case VM_FFI_TYPE_INT8:
        {
            return nanbox_from_double((double)*(int8_t *)value);
        }
        case VM_FFI_TYPE_INT16:
        {
            return nanbox_from_double((double)*(int16_t *)value);
        }
        case VM_FFI_TYPE_INT32:
        {
            return nanbox_from_double((double)*(int32_t *)value);
        }
        case VM_FFI_TYPE_INT64:
        {
            return nanbox_from_double((double)*(int64_t *)value);
        }
        case VM_FFI_TYPE_UINT8:
        {
            return nanbox_from_double((double)*(uint8_t *)value);
        }
        case VM_FFI_TYPE_UINT16:
        {
            return nanbox_from_double((double)*(uint16_t *)value);
        }
        case VM_FFI_TYPE_UINT32:
        {
            return nanbox_from_double((double)*(uint32_t *)value);
        }
        case VM_FFI_TYPE_UINT64:
        {
            return nanbox_from_double((double)*(uint64_t *)value);
        }
        case VM_FFI_TYPE_FLOAT32:
        {
            return nanbox_from_double((double)*(float *)value);
        }
        case VM_FFI_TYPE_FLOAT64:
        {
            return nanbox_from_double((double)*(double *)value);
        }
        case VM_FFI_TYPE_STRING:
        {
            return vm_ffi_from_string(gc, *(char**)value);
        }
        default:
            printf("internal error: cast output: unknown enum\n");
            exit(0);
        }
    }
    else
    {
        printf("internal error: cast output: no generics yet\n");
        exit(0);
    }
}

vm_ffi_res_t vm_ffi_opcode(vm_gc_t *gc, nanbox_t library, nanbox_t function, nanbox_t retty, nanbox_t argty, nanbox_t arguments)
{
    void *handle;
    if (vm_gc_sizeof(gc, library) == 0)
    {
        handle = dlopen(NULL, RTLD_LAZY);
    }
    else
    {
        char *filename = vm_ffi_tostring(gc, library);
        handle = dlopen(filename, RTLD_LAZY | RTLD_NODELETE);
        vm_mem_free(filename);
    }
    if (handle == NULL)
    {
        return (vm_ffi_res_t){
            .state = VM_FFI_ERROR_OPENING,
        };
    }
    char *entry = vm_ffi_tostring(gc, function);
    void *symbol = dlsym(handle, entry);
    vm_mem_free(entry);
    if (symbol == NULL)
    {
        return (vm_ffi_res_t){
            .state = VM_FFI_ERROR_SYMBOL,
        };
    }
    ffi_type *fretty = vm_ffi_get_type(gc, retty);
    int argc = vm_gc_sizeof(gc, argty);
    ffi_type *fargty[argc];
    for (int argno = 0; argno < argc; argno++)
    {
        nanbox_t nargty = vm_gc_get(gc, argty, argno);
        fargty[argno] = vm_ffi_get_type(gc, nargty);
    }
    ffi_cif cif;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argc, fretty, fargty) != FFI_OK)
    {
        return (vm_ffi_res_t){
            .state = VM_FFI_ERROR_CIF,
        };
    }
    void *retval = vm_mem_alloc(vm_ffi_type_size(gc, retty));
    void **args = vm_mem_alloc(sizeof(void *) * argc);
    for (int argno = 0; argno < argc; argno++)
    {
        nanbox_t nargty = vm_gc_get(gc, argty, argno);
        nanbox_t nargval = vm_gc_get(gc, arguments, argno);
        args[argno] = vm_ffi_cast_input(gc, nargty, nargval);
    }
    ffi_call(&cif, symbol, retval, args);
    for (int argno = 0; argno < argc; argno++)
    {
        vm_mem_free(args[argno]);
    }
    vm_mem_free(args);
    nanbox_t retbox = vm_ffi_cast_output(gc, retty, retval);
    vm_mem_free(retval);
    return (vm_ffi_res_t){
        .state = VM_FFI_NO_ERROR,
        .result = retbox,
    };
}