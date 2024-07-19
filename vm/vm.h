
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VM_VERSION "0.0.5"

#define VM_GC_MIN 256
#define VM_GC_FACTOR 1.4

#define VM_FORMAT_FLOAT "%.14g"

struct vm_t;
struct vm_block_t;
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
    VM_TAG_NUMBER,
    VM_TAG_STR,
    VM_TAG_CLOSURE,
    VM_TAG_FUN,
    VM_TAG_TAB,
    VM_TAG_FFI,
    VM_TAG_ERROR,
    VM_TAG_MAX,
};

typedef uint8_t vm_tag_t;

typedef void vm_ffi_t(vm_t *closure, vm_obj_t *args);

union vm_value_t {
    void *all;
    bool b;
    double f64;
    vm_ffi_t *ffi;
    vm_io_buffer_t *str;
    vm_table_t *table;
    vm_closure_t *closure;
    struct vm_block_t *fun;
    struct vm_error_t *error;
};

struct vm_obj_t {
    vm_value_t value;
    vm_tag_t tag;
};

struct vm_table_pair_t {
    vm_obj_t key;
    vm_obj_t value;
};

struct vm_table_t {
    vm_table_pair_t *pairs;
    uint32_t len;
    uint32_t used;
    uint8_t alloc: 8;
    bool mark: 1;
    bool pairs_auto: 1;
};

struct vm_closure_t {
    struct vm_block_t *block;
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

void vm_repl(vm_t *vm);
vm_obj_t vm_str(vm_t *vm, const char *str);

#define vm_obj_of_empty() ((vm_obj_t) {.tag = VM_TAG_UNK})
#define vm_obj_of_nil() ((vm_obj_t) {.tag = VM_TAG_NIL})
#define vm_obj_of_bool(b) ((vm_obj_t) {.tag = VM_TAG_BOOL, .value.b = (b)})
#define vm_obj_of_number(n) ((vm_obj_t) {.tag = VM_TAG_NUMBER, .value.f64 = (n)})
#define vm_obj_of_str(o) ((vm_obj_t) {.tag = VM_TAG_STR, .value.str = (o)})
#define vm_obj_of_table(o) ((vm_obj_t) {.tag = VM_TAG_TAB, .value.table = (o)})
#define vm_obj_of_closure(o) ((vm_obj_t) {.tag = VM_TAG_CLOSURE, .value.closure = (o)})
#define vm_obj_of_ffi(o) ((vm_obj_t) {.tag = VM_TAG_FFI, .value.ffi = (o)})
#define vm_obj_of_block(o) ((vm_obj_t) {.tag = VM_TAG_FUN, .value.fun = (o)})
#define vm_obj_of_error(o) ((vm_obj_t) {.tag = VM_TAG_FUN, .value.error = (o)})

#endif
