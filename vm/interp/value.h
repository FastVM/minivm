#if !defined(VM_HEADER_BE_VALUE)
#define VM_HEADER_BE_VALUE

#include "../ir.h"

union vm_value_t;
typedef union vm_value_t vm_value_t;

union vm_value_t {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    void *func;
    void *lib;
    void *sym;
    const char *name;
};

#endif
