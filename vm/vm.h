
#if !defined(VM_HEADER_VM)
#define VM_HEADER_VM

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VM_VERSION "0.0.5"

#define VM_NREGS 256

#define VM_GC_MIN (1 << 14)
// #define VM_GC_MIN 16
#define VM_GC_FACTOR 1.5

#define VM_DEBUG_BACKEND_BLOCKS 0
#define VM_DEBUG_BACKEND_OPCODES 0

#define VM_FORMAT_FLOAT "%.14g"

#define VM_USE_SPALL 0
#define VM_USE_SPALL_INSTR 0

#define VM_OBJ_FAST 1
#define VM_OBJ_FIELD_VALUE _value ## __COUNTER__ 
#define VM_OBJ_FIELD_TAG _tag ## __COUNTER__

struct vm_t;
struct vm_block_t;
struct vm_blocks_t;
struct vm_closure_t;
struct vm_externs_t;
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
typedef union vm_value_t vm_value_t;
typedef struct vm_table_t vm_table_t;
typedef struct vm_table_pair_t vm_table_pair_t;

#if VM_OBJ_FAST

#include "nanbox.h"

#define VM_EMPTY_BYTE NANBOX_EMPTY_BYTE

typedef nanbox_t vm_obj_t;

typedef void vm_ffi_t(vm_t *closure, vm_obj_t *args);
#define vm_obj_of_empty() nanbox_empty()
#define vm_obj_of_nil() nanbox_undefined()
#define vm_obj_of_boolean(b) ((b) ? nanbox_true() : nanbox_false()) 
#define vm_obj_of_number(n) nanbox_from_double(n)
#define vm_obj_of_buffer(s) nanbox_from_aux1(s)
#define vm_obj_of_table(o) nanbox_from_aux2(o)
#define vm_obj_of_closure(o) nanbox_from_aux3(o)
#define vm_obj_of_ffi(o) nanbox_from_aux4(o)
#define vm_obj_of_block(o) nanbox_from_aux5(o)
#define vm_obj_of_error(o) nanbox_from_pointer(o)

#define vm_obj_is_empty(o) nanbox_is_empty(o)
#define vm_obj_is_nil(o) nanbox_is_undefined(o)
#define vm_obj_is_boolean(o) nanbox_is_boolean(o)
#define vm_obj_is_number(o) nanbox_is_number(o)
#define vm_obj_is_string(o) nanbox_is_aux1(o)
#define vm_obj_is_table(o) nanbox_is_aux2(o)
#define vm_obj_is_closure(o) nanbox_is_aux3(o)
#define vm_obj_is_ffi(o) nanbox_is_aux4(o)
#define vm_obj_is_block(o) nanbox_is_aux5(o)
#define vm_obj_is_error(o) nanbox_is_pointer(o)

#define vm_obj_get_boolean(o) nanbox_to_boolean(o)
#define vm_obj_get_number(o) nanbox_to_double(o)
#define vm_obj_get_string(o) ((vm_io_buffer_t *) nanbox_to_aux(o))
#define vm_obj_get_table(o) ((vm_table_t *) nanbox_to_aux(o))
#define vm_obj_get_closure(o) ((vm_closure_t *) nanbox_to_aux(o))
#define vm_obj_get_ffi(o) ((vm_ffi_t *) nanbox_to_aux(o))
#define vm_obj_get_block(o) ((vm_block_t *) nanbox_to_aux(o))
#define vm_obj_get_error(o) ((vm_error_t *) nanbox_to_pointer(o))

#else

struct vm_obj_t;
typedef struct vm_obj_t vm_obj_t;

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
    bool boolean;
    double f64;
    vm_ffi_t *ffi;
    vm_io_buffer_t *str;
    vm_table_t *table;
    vm_closure_t *closure;
    struct vm_block_t *fun;
    struct vm_error_t *error;
};

struct vm_obj_t {
    vm_value_t VM_OBJ_FIELD_VALUE;
    vm_tag_t VM_OBJ_FIELD_TAG;
};
#endif

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
    vm_externs_t *externs;
    vm_blocks_t *blocks;

    uint8_t use_num;

    vm_obj_t std;

    void *gc;

    vm_obj_t *base;
    vm_obj_t *regs;

    uint32_t nblocks;

    bool dump_ir: 1;
};

vm_t *vm_state_new(void);
void vm_state_delete(vm_t *vm);

void vm_repl(vm_t *vm);
vm_obj_t vm_obj_of_string(vm_t *vm, const char *str);

#if VM_OBJ_FAST
#else
#define VM_EMPTY_BYTE 0x00

#define vm_obj_of_empty() ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_UNK})
#define vm_obj_of_nil() ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_NIL})
#define vm_obj_of_boolean(b) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_BOOL, .VM_OBJ_FIELD_VALUE.boolean = (b)})
#define vm_obj_of_number(n) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_NUMBER, .VM_OBJ_FIELD_VALUE.f64 = (n)})
#define vm_obj_of_string(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_STR, .VM_OBJ_FIELD_VALUE.str = (o)})
#define vm_obj_of_table(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_TAB, .VM_OBJ_FIELD_VALUE.table = (o)})
#define vm_obj_of_closure(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_CLOSURE, .VM_OBJ_FIELD_VALUE.closure = (o)})
#define vm_obj_of_ffi(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_FFI, .VM_OBJ_FIELD_VALUE.ffi = (o)})
#define vm_obj_of_block(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_FUN, .VM_OBJ_FIELD_VALUE.fun = (o)})
#define vm_obj_of_error(o) ((vm_obj_t) {.VM_OBJ_FIELD_TAG = VM_TAG_FUN, .VM_OBJ_FIELD_VALUE.error = (o)})

#define vm_obj_get_boolean(o) ((o).VM_OBJ_FIELD_VALUE.boolean)
#define vm_obj_get_number(o) ((o).VM_OBJ_FIELD_VALUE.f64)
#define vm_obj_get_string(o) ((o).VM_OBJ_FIELD_VALUE.str)
#define vm_obj_get_table(o) ((o).VM_OBJ_FIELD_VALUE.table)
#define vm_obj_get_closure(o) ((o).VM_OBJ_FIELD_VALUE.closure)
#define vm_obj_get_ffi(o) ((o).VM_OBJ_FIELD_VALUE.ffi)
#define vm_obj_get_block(o) ((o).VM_OBJ_FIELD_VALUE.fun)
#define vm_obj_get_error(o) ((o).VM_OBJ_FIELD_VALUE.error)

#define vm_obj_is_empty(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_UNK)
#define vm_obj_is_nil(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_NIL)
#define vm_obj_is_boolean(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_BOOL)
#define vm_obj_is_number(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_NUMBER)
#define vm_obj_is_string(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_STR)
#define vm_obj_is_table(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_TAB)
#define vm_obj_is_closure(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_CLOSURE)
#define vm_obj_is_ffi(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_FFI)
#define vm_obj_is_block(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_FUN)
#define vm_obj_is_error(o) ((o).VM_OBJ_FIELD_TAG == VM_TAG_ERROR)
#endif

#endif
