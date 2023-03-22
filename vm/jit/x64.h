#if !defined(VM_HEADER_BE_X64)
#define VM_HEADER_BE_X64

#include "../lib.h"
#include "../ir.h"
#include "../type.h"

struct vm_x64_instr_t;
typedef struct vm_x64_instr_t vm_x64_instr_t;

struct vm_x64_arg_t;
typedef struct vm_x64_arg_t vm_x64_arg_t;

struct vm_x64_comp_t;
typedef struct vm_x64_comp_t vm_x64_comp_t;

struct vm_x64_cache_t;
typedef struct vm_x64_cache_t vm_x64_cache_t;

typedef uint8_t vm_x64_arg_type_t;
enum {
    VM_X64_ARG_REG,
    VM_X64_ARG_LOAD,
    VM_X64_ARG_LABEL,
    VM_X64_ARG_IMM32,
    VM_X64_ARG_IMM64,
};

typedef uint16_t vm_x64_opcode_t;
enum {
    VM_X64_OPCODE_CMP,
    VM_X64_OPCODE_TEST,
    VM_X64_OPCODE_JMP,
    VM_X64_OPCODE_JE,
    VM_X64_OPCODE_JL,
    VM_X64_OPCODE_JB,
    VM_X64_OPCODE_ADD,
    VM_X64_OPCODE_SUB,
    VM_X64_OPCODE_MUL,
    VM_X64_OPCODE_DIV,
    VM_X64_OPCODE_MOV,
    VM_X64_OPCODE_PUSH,
    VM_X64_OPCODE_POP,
    VM_X64_OPCODE_CALL,
    VM_X64_OPCODE_XOR,
    VM_X64_OPCODE_MAX_OPCODE,
};

struct vm_x64_arg_t {
    vm_x64_arg_type_t type;
    union {
        uint8_t reg;
        uint8_t load;
        uint32_t imm32;
        uint64_t imm64;
        uint32_t label;
    };
};

struct vm_x64_instr_t {
    vm_x64_instr_t *next;
    vm_x64_instr_t *last;
    vm_x64_opcode_t opcode;
    uint64_t bitsize;
    vm_x64_arg_t args[2];
};

struct vm_x64_comp_t {
    size_t ncomps;
};

struct vm_x64_cache_t {
    uint16_t args[256];
    vm_block_t *block;
};

extern const uint8_t vm_x64_instr_argc_table[VM_X64_OPCODE_MAX_OPCODE];
void vm_x64_instr_argc(vm_x64_opcode_t instr);
void vm_x64_print_opcode(FILE *file, vm_x64_opcode_t opcode);
void vm_x64_print_reg(FILE *file, uint8_t num, uint8_t bitsize);
void vm_x64_print_instr(FILE *file, vm_x64_instr_t instr);

void vm_x64_instr_insert_after(vm_x64_instr_t *after_me, vm_x64_instr_t *instr);
void vm_x64_instr_insert_before(vm_x64_instr_t *before_me, vm_x64_instr_t *instr);
void vm_x64_instr_insert1_after(vm_x64_instr_t *after_me, vm_x64_instr_t instr);
void vm_x64_instr_insert1_before(vm_x64_instr_t *before_me, vm_x64_instr_t instr);

vm_x64_cache_t *vm_x64_cache_new(void);

vm_block_t *vm_x64_rblock_version(vm_x64_comp_t *comp, vm_rblock_t *rblock);
void vm_x64_run(vm_block_t *block);

#endif