
#if !defined(VM_HEADER_IR_BUILD)
#define VM_HEADER_IR_BUILD

#include "lib.h"
#include "ir.h"

void vm_instr_free(vm_instr_t *instr);
void vm_block_free(vm_block_t *block);
void vm_blocks_free(size_t nblocks, vm_block_t *blocks);

vm_arg_t vm_arg_nil(void);
vm_arg_t vm_arg_bool(bool t);
vm_arg_t vm_arg_reg(size_t reg);
vm_arg_t vm_arg_extern(size_t reg);
vm_arg_t vm_arg_num(double num);
vm_arg_t vm_arg_str(const char *str);
vm_arg_t vm_arg_func(vm_block_t *func);

void vm_block_realloc(vm_block_t *block, vm_instr_t *instr);

void vm_block_add_move(vm_block_t *block, vm_arg_t out, vm_arg_t arg);
void vm_block_add_add(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_sub(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_mul(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_div(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_mod(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_bor(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_band(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_bxor(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_bshr(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_bshl(vm_block_t *block, vm_arg_t out, vm_arg_t lhs, vm_arg_t rhs);
void vm_block_add_xcall(vm_block_t *block, vm_arg_t out, vm_arg_t func, size_t nargs, vm_arg_t *args);
void vm_block_add_call(vm_block_t *block, vm_arg_t out, vm_arg_t func, size_t nargs, vm_arg_t *args);
void vm_block_add_arr(vm_block_t *block, vm_arg_t out, vm_arg_t num);
void vm_block_add_tab(vm_block_t *block, vm_arg_t out);
void vm_block_add_get(vm_block_t *block, vm_arg_t out, vm_arg_t obj, vm_arg_t index);
void vm_block_add_len(vm_block_t *block, vm_arg_t out, vm_arg_t obj);
void vm_block_add_type(vm_block_t *block, vm_arg_t out, vm_arg_t obj);
void vm_block_add_set(vm_block_t *block, vm_arg_t obj, vm_arg_t index, vm_arg_t value);
void vm_block_add_out(vm_block_t *block, vm_arg_t val);
void vm_block_add_in(vm_block_t *block, vm_arg_t val);

void vm_block_end_jump(vm_block_t *block, vm_block_t *target);
void vm_block_end_bb(vm_block_t *block, vm_arg_t val, vm_block_t *iffalse, vm_block_t *iftrue);
void vm_block_end_blt(vm_block_t *block, vm_arg_t lhs, vm_arg_t rhs, vm_block_t *iffalse,
                         vm_block_t *iftrue);
void vm_block_end_beq(vm_block_t *block, vm_arg_t lhs, vm_arg_t rhs, vm_block_t *iffalse,
                         vm_block_t *iftrue);
void vm_block_end_ret(vm_block_t *block, vm_arg_t arg);
void vm_block_end_exit(vm_block_t *block);

void vm_print_arg(FILE *out, vm_arg_t val);
void vm_print_branch(FILE *out, vm_branch_t *val);
void vm_print_instr(FILE *out, vm_instr_t *val);
void vm_print_block(FILE *out, vm_block_t *val);
void vm_print_blocks(FILE *out, size_t nblocks, vm_block_t *val);

void vm_info(size_t *nops, vm_block_t **blocks);

#endif
