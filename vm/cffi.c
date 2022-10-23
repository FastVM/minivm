
#include "./cffi.h"

#include "../../ffi/include/ffi.h"
#include "./tag.h"

#if defined(_WIN32)
static HMODULE vm_ffi_current_module(void) {
    HMODULE win = NULL;

    GetModuleHandleEx(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCTSTR)&vm_ffi_current_module,
        &win
    );

    return win;
}
#endif

vm_ffi_handle_t *vm_ffi_handle_open(const char *filename) {
    vm_ffi_handle_t *ret = vm_malloc(sizeof(vm_ffi_handle_t));
#if defined(_WIN32)
    if (filename == NULL) {
        ret->win = vm_ffi_current_module();
    } else {
        ret->win = LoadLibraryA(filename);
    }
    if (ret->win == NULL) {
        fprintf(stderr, "failed to load lib: `%s` (windows)\n", filename);
    }
#elif defined(_POSIX_VERSION)
    ret->pos = fopen(filename, RTLD_LAZY);
    if (ret->pos == NULL) {
        fprintf(stderr, "failed to load lib: `%s` (posix)\n", filename);
    }
#endif
    return ret;
}

void vm_ffi_handle_close(vm_ffi_handle_t *handle) {
#if defined(_WIN32)
    FreeLibrary(handle->win);
#elif defined(_POSIX_VERSION)
    dlclose(handle->pos);
#endif
    vm_free(handle);
}

static ffi_type *vm_tag_to_ffi_type(vm_tag_t tag) {
    if (tag.type == VM_TAG_TYPE_NIL) {
        return &ffi_type_void;
    }
    if (tag.type == VM_TAG_TYPE_SINT) {
        if (tag.size == sizeof(int8_t)) return &ffi_type_sint8;
        if (tag.size == sizeof(int16_t)) return &ffi_type_sint16;
        if (tag.size == sizeof(int32_t)) return &ffi_type_sint32;
        if (tag.size == sizeof(int64_t)) return &ffi_type_sint64;
        __builtin_trap();
    }
    if (tag.type == VM_TAG_TYPE_SINT) {
        if (tag.size == sizeof(uint8_t)) return &ffi_type_uint8;
        if (tag.size == sizeof(uint16_t)) return &ffi_type_uint16;
        if (tag.size == sizeof(uint32_t)) return &ffi_type_uint32;
        if (tag.size == sizeof(uint64_t)) return &ffi_type_uint64;
        __builtin_trap();
    }
    if (tag.type == VM_TAG_TYPE_FLOAT) {
        if (tag.size == sizeof(float)) return &ffi_type_float;
        if (tag.size == sizeof(double)) return &ffi_type_double;
        __builtin_trap();
    }
    __builtin_trap();
}

vm_ffi_symbol_t *vm_ffi_handle_get(vm_ffi_handle_t *handle, const char *str, vm_tag_t ret, size_t nargs, vm_tag_t *args) {
#if defined(_WIN32)
    void (*ptr)(void) = (void *)GetProcAddress(handle->win, str);
#elif defined(_POSIX_VERSION)
    void (*ptr)(void) = (void *)dlsym(handle->pos, str);
#endif
    ffi_type *atypes[17];
    for (size_t i = 0; i < nargs; i++) {
        atypes[i] = vm_tag_to_ffi_type(args[i]);
    }
    atypes[nargs] = NULL;
    ffi_cif *cif = vm_malloc(sizeof(ffi_cif));
    ffi_prep_cif(cif, FFI_DEFAULT_ABI, nargs, vm_tag_to_ffi_type(ret), atypes);
    vm_ffi_symbol_t *sym = vm_malloc(sizeof(vm_ffi_symbol_t));
    sym->cif = cif;
    sym->func = ptr;
    return sym;
}

vm_value_t vm_ffi_symbol_call(vm_ffi_symbol_t *sym, size_t nargs, vm_value_t *args) {
    vm_value_t ret;
    ffi_call(sym->cif, sym->func, &ret, (void**) args);
    return ret;
}
