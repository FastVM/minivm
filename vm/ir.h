
#if !defined(VM_HEADER_IR)
#define VM_HEADER_IR

struct vm_ir_arg_t;
struct vm_ir_block_t;
struct vm_ir_branch_t;
struct vm_ir_instr_t;

typedef struct vm_ir_arg_t vm_ir_arg_t;
typedef struct vm_ir_block_t vm_ir_block_t;
typedef struct vm_ir_branch_t vm_ir_branch_t;
typedef struct vm_ir_instr_t vm_ir_instr_t;

#include "lib.h"
#include "io.h"
#include "errors.h"

enum {
    // there are no more args
    VM_IR_ARG_TYPE_NONE,
    // there was an error generating this arg
    VM_IR_ARG_TYPE_ERROR,
    // normal args
    VM_IR_ARG_TYPE_REG,
    VM_IR_ARG_TYPE_LIT,
};

enum {
    VM_IR_BRANCH_OPCODE_FALL,
    VM_IR_BRANCH_OPCODE_JUMP,
    VM_IR_BRANCH_OPCODE_BB,
    VM_IR_BRANCH_OPCODE_BLT,
    VM_IR_BRANCH_OPCODE_BLE,
    VM_IR_BRANCH_OPCODE_BEQ,
    VM_IR_BRANCH_OPCODE_RET,
    VM_IR_BRANCH_OPCODE_LOAD,
    VM_IR_BRANCH_OPCODE_GET,
    VM_IR_BRANCH_OPCODE_CALL,
};

enum {
    VM_IR_INSTR_OPCODE_NOP,
    VM_IR_INSTR_OPCODE_MOVE,
    VM_IR_INSTR_OPCODE_ADD,
    VM_IR_INSTR_OPCODE_SUB,
    VM_IR_INSTR_OPCODE_MUL,
    VM_IR_INSTR_OPCODE_DIV,
    VM_IR_INSTR_OPCODE_IDIV,
    VM_IR_INSTR_OPCODE_MOD,
    VM_IR_INSTR_OPCODE_TABLE_SET,
    VM_IR_INSTR_OPCODE_TABLE_NEW,
    VM_IR_INSTR_OPCODE_TABLE_LEN,
    VM_IR_INSTR_OPCODE_STD,
};

struct vm_ir_arg_t {
    union {
        vm_obj_t lit;
        vm_ir_block_t *func;
        vm_error_t *error;

        uint64_t reg;
    };

    uint8_t type;
};

struct vm_ir_branch_t {
    vm_location_range_t range;

    vm_ir_block_t *targets[2];

    vm_ir_arg_t *args;
    vm_ir_arg_t out;

    uint8_t op;
};

struct vm_ir_instr_t {
    vm_location_range_t range;
    
    vm_ir_arg_t *args;
    vm_ir_arg_t out;

    uint8_t op;
};

struct vm_ir_block_t {
    vm_location_range_t range;
    
    uint32_t id;

    uint32_t alloc;
    uint32_t len;
    vm_ir_instr_t *instrs;

    vm_ir_branch_t branch;

    void *code;

    bool isfunc : 1;
    bool mark: 1;
};


struct vm_ir_blocks_t {
    vm_ir_block_t *block;
    vm_ir_blocks_t *next;
};

void vm_block_realloc(vm_ir_block_t *block, vm_ir_instr_t instr);

void vm_io_format_arg(vm_io_buffer_t *out, vm_ir_arg_t val);
void vm_io_format_branch(vm_io_buffer_t *out, vm_ir_branch_t val);
void vm_io_format_instr(vm_io_buffer_t *out, vm_ir_instr_t val);
void vm_io_format_block(vm_io_buffer_t *out, vm_ir_block_t *val);

#define vm_arg_nil() ((vm_ir_arg_t){.type = (VM_IR_ARG_TYPE_LIT), .lit = vm_obj_of_nil()})

vm_ir_block_t *vm_lang_lua_compile(vm_t *vm, const char *src, const char *file);

#endif
