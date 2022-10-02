
#if !defined(VM_HEADER_IR_BUILD)
#define VM_HEADER_IR_BUILD

#include "../lib.h"
#include "ir.h"

#define vm_ir_dup(v)                              \
    ({                                            \
        __typeof__(v) *k_ = vm_alloc0(sizeof(v)); \
        *k = (v);                                 \
        k;                                        \
    })
#define vm_ir_new(t_, ...)             \
    ({                                 \
        t_ *k = vm_alloc0(sizeof(t_)); \
        *k = (t_){__VA_ARGS__};        \
        k;                             \
    })

void vm_ir_instr_free(vm_ir_instr_t *instr);
void vm_ir_block_free(vm_ir_block_t *block);
void vm_ir_blocks_free(size_t nblocks, vm_ir_block_t *blocks);

vm_ir_arg_t vm_ir_arg_nil(void);
vm_ir_arg_t vm_ir_arg_bool(bool t);
vm_ir_arg_t vm_ir_arg_reg(size_t reg);
vm_ir_arg_t vm_ir_arg_extern(size_t reg);
vm_ir_arg_t vm_ir_arg_num(double num);
vm_ir_arg_t vm_ir_arg_str(const char *str);
vm_ir_arg_t vm_ir_arg_func(vm_ir_block_t *func);

void vm_ir_block_realloc(vm_ir_block_t *block, vm_ir_instr_t *instr);

void vm_ir_block_add_move(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t arg);
void vm_ir_block_add_add(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_sub(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_mul(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_div(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_mod(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_bor(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_band(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_bxor(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_bshr(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_bshl(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t lhs, vm_ir_arg_t rhs);
void vm_ir_block_add_xcall(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t func, size_t nargs, vm_ir_arg_t *args);
void vm_ir_block_add_call(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t func, size_t nargs, vm_ir_arg_t *args);
void vm_ir_block_add_arr(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t num);
void vm_ir_block_add_tab(vm_ir_block_t *block, vm_ir_arg_t out);
void vm_ir_block_add_get(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t obj, vm_ir_arg_t index);
void vm_ir_block_add_len(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t obj);
void vm_ir_block_add_type(vm_ir_block_t *block, vm_ir_arg_t out, vm_ir_arg_t obj);
void vm_ir_block_add_set(vm_ir_block_t *block, vm_ir_arg_t obj, vm_ir_arg_t index, vm_ir_arg_t value);
void vm_ir_block_add_out(vm_ir_block_t *block, vm_ir_arg_t val);
void vm_ir_block_add_in(vm_ir_block_t *block, vm_ir_arg_t val);

void vm_ir_block_end_jump(vm_ir_block_t *block, vm_ir_block_t *target);
void vm_ir_block_end_bb(vm_ir_block_t *block, vm_ir_arg_t val, vm_ir_block_t *iffalse, vm_ir_block_t *iftrue);
void vm_ir_block_end_blt(vm_ir_block_t *block, vm_ir_arg_t lhs, vm_ir_arg_t rhs, vm_ir_block_t *iffalse,
                         vm_ir_block_t *iftrue);
void vm_ir_block_end_beq(vm_ir_block_t *block, vm_ir_arg_t lhs, vm_ir_arg_t rhs, vm_ir_block_t *iffalse,
                         vm_ir_block_t *iftrue);
void vm_ir_block_end_ret(vm_ir_block_t *block, vm_ir_arg_t arg);
void vm_ir_block_end_exit(vm_ir_block_t *block);

void vm_ir_print_arg(FILE *out, vm_ir_arg_t val);
void vm_ir_print_branch(FILE *out, vm_ir_branch_t *val);
void vm_ir_print_instr(FILE *out, vm_ir_instr_t *val);
void vm_ir_print_block(FILE *out, vm_ir_block_t *val);
void vm_ir_print_blocks(FILE *out, size_t nblocks, vm_ir_block_t *val);

void vm_ir_info(size_t *nops, vm_ir_block_t **blocks);

#endif
