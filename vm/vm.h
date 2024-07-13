
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define VM_GC_FACTOR 1.03659
#define VM_GC_FACTOR 1.25
#define VM_FORMAT_FLOAT "%.14g"

struct vm_t;
struct vm_blocks_t;
struct vm_closure_t;
struct vm_externs_t;
struct vm_obj_t;
union vm_value_t;
struct vm_table_t;
struct vm_table_pair_t;
struct vm_table_pair_t;
struct vm_io_buffer_t;

typedef struct vm_io_buffer_t vm_io_buffer_t;
typedef struct vm_t vm_t;
typedef struct vm_blocks_t vm_blocks_t;
typedef struct vm_closure_t vm_closure_t;
typedef struct vm_externs_t vm_externs_t;
typedef struct vm_obj_t vm_obj_t;
typedef union vm_value_t vm_value_t;
typedef struct vm_table_t vm_table_t;
typedef struct vm_table_pair_t vm_table_pair_t;

enum {
    VM_TAG_UNK,
    VM_TAG_NIL,
    VM_TAG_BOOL,
    VM_TAG_I8,
    VM_TAG_I16,
    VM_TAG_I32,
    VM_TAG_I64,
    VM_TAG_F32,
    VM_TAG_F64,
    VM_TAG_STR,
    VM_TAG_CLOSURE,
    VM_TAG_FUN,
    VM_TAG_TAB,
    VM_TAG_FFI,
    VM_TAG_ERROR,
    VM_TAG_MAX,
};

enum {
    VM_USE_NUM_I8,
    VM_USE_NUM_I16,
    VM_USE_NUM_I32,
    VM_USE_NUM_I64,
    VM_USE_NUM_F32,
    VM_USE_NUM_F64,
};

typedef uint8_t vm_tag_t;

typedef void vm_ffi_t(vm_t *closure, vm_obj_t *args);

union vm_value_t {
    void *all;
    bool b;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    vm_io_buffer_t *str;
    vm_table_t *table;
    vm_closure_t *closure;
    vm_ffi_t *ffi;
    struct vm_error_t *error;
};

struct vm_obj_t {
    vm_value_t value;
    vm_tag_t tag;
};

struct vm_table_pair_t {
    vm_value_t key_val;
    vm_value_t val_val;
    vm_tag_t key_tag;
    vm_tag_t val_tag;
};

struct vm_table_t {
    vm_table_pair_t *pairs;
    uint32_t len;
    uint32_t used;
    uint8_t alloc;
    bool mark: 1;
};

struct vm_closure_t {
    bool mark: 1;
    uint32_t len: 31;
    vm_obj_t values[];
};

struct vm_externs_t {
    size_t id;
    void *value;
    vm_externs_t *last;
};

struct vm_io_buffer_t {
    char *buf;
    bool mark: 1;
    uint32_t len: 31;
    uint32_t alloc: 32;
};

struct vm_t {
    const char *save_file;
    vm_externs_t *externs;
    vm_blocks_t *blocks;

    uint8_t use_num;

    vm_obj_t std;

    void *gc;

    vm_obj_t *base;
    vm_obj_t *regs;

    bool dump_ir: 1;
};

vm_t *vm_state_new(void);
void vm_state_delete(vm_t *vm);

vm_obj_t vm_state_load(vm_t *vm, const char *str, const char *filename);
vm_obj_t vm_state_invoke(vm_t *vm, vm_obj_t obj, size_t nargs, vm_obj_t *args);

void vm_repl(vm_t *config);
vm_obj_t vm_str(vm_t *vm, const char *str);

#endif
