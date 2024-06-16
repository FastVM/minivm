
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VM_FORMAT_FLOAT "%.14g"

struct vm_blocks_t;
struct vm_config_t;
struct vm_externs_t;
struct vm_std_closure_t;

typedef struct vm_blocks_t vm_blocks_t;
typedef struct vm_config_t vm_config_t;
typedef struct vm_externs_t vm_externs_t;
typedef struct vm_std_closure_t vm_std_closure_t;

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

struct vm_externs_t {
    size_t id;
    void *value;
    vm_externs_t *last;
};

struct vm_config_t {
    const char *save_file;
    vm_externs_t *externs;
    vm_blocks_t *blocks;

    uint8_t use_num;
};

#endif
