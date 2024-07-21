
#include "../gc.h"
#include "../ir.h"
#include "../obj.h"
#include "../vm.h"

#define VM_INLINE inline __attribute__((always_inline)) __attribute__((no_instrument_function))

#if VM_USE_SPALL_INSTR

#include "../../vendor/spall/auto.h"

static int x = 0;

#define VM_OPCODE_SPALL_BEGIN(s) \
    x++;                         \
    spall_auto_buffer_begin(#s, strlen(#s), "", 0)
#define VM_OPCODE_SPALL_END() \
    x--;                      \
    spall_auto_buffer_end()

#else

#define VM_OPCODE_SPALL_BEGIN(s)
#define VM_OPCODE_SPALL_END()

#endif

#if f
#define VM_OPCODE_DEBUG(s) VM_OPCODE_SPALL_BEGIN(s); printf("%s\n", #s);
#else
#define VM_OPCODE_DEBUG(s) VM_OPCODE_SPALL_BEGIN(s);
#endif

#define vm_backend_return(v) ({ \
    vm_obj_t return_ = (v);     \
    VM_OPCODE_SPALL_END();      \
    return v;                   \
})
#define vm_run_repl_jump() VM_OPCODE_SPALL_END(); goto *vm_run_repl_read(void *)
#define vm_backend_new_block() VM_OPCODE_SPALL_END(); goto new_block

typedef uint8_t vm_interp_tag_t;
typedef uint32_t vm_interp_reg_t;

enum {
    VM_OP_TABLE_SET,
    VM_OP_TABLE_NEW,
    VM_OP_TABLE_LEN,

    VM_OP_MOVE_I,
    VM_OP_MOVE_R,

    VM_OP_ADD_RI,
    VM_OP_ADD_IR,
    VM_OP_ADD_RR,

    VM_OP_SUB_RI,
    VM_OP_SUB_IR,
    VM_OP_SUB_RR,

    VM_OP_MUL_RI,
    VM_OP_MUL_IR,
    VM_OP_MUL_RR,

    VM_OP_DIV_RI,
    VM_OP_DIV_IR,
    VM_OP_DIV_RR,

    VM_OP_IDIV_RI,
    VM_OP_IDIV_IR,
    VM_OP_IDIV_RR,

    VM_OP_MOD_RI,
    VM_OP_MOD_IR,
    VM_OP_MOD_RR,

    VM_OP_JUMP,

    VM_OP_BB_R,

    VM_OP_BLT_RI,
    VM_OP_BLT_IR,
    VM_OP_BLT_RR,

    VM_OP_BLE_RI,
    VM_OP_BLE_IR,
    VM_OP_BLE_RR,

    VM_OP_BEQ_RI,
    VM_OP_BEQ_IR,
    VM_OP_BEQ_RR,

    VM_OP_RET_I,
    VM_OP_RET_R,

    VM_OP_LOAD,
    VM_OP_GET,
    VM_OP_CALL,

    VM_MAX_OP,
};

static VM_INLINE bool vm_interp_value_eq(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_nil(v1) && vm_obj_is_nil(v2)) {
        return true;
    } else if (vm_obj_is_boolean(v1) && vm_obj_is_boolean(v2)) {
        return vm_obj_get_boolean(v1) == vm_obj_get_boolean(v2);
    } else if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) == vm_obj_get_number(v2);
    } else if (vm_obj_is_string(v1) && vm_obj_is_string(v2)) {
        return strcmp(vm_obj_get_string(v1)->buf, vm_obj_get_string(v2)->buf) == 0;
    } else if (vm_obj_is_table(v1) && vm_obj_is_table(v2)) {
        return vm_obj_get_table(v1) == vm_obj_get_table(v2);
    } else if (vm_obj_is_closure(v1) && vm_obj_is_closure(v2)) {
        return vm_obj_get_closure(v1) == vm_obj_get_closure(v2);
    } else if (vm_obj_is_ffi(v1) && vm_obj_is_ffi(v2)) {
        return vm_obj_get_ffi(v1) == vm_obj_get_ffi(v2);
    } else {
        __builtin_trap();
        return false;
    }
}

static VM_INLINE bool vm_interp_value_lt(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) < vm_obj_get_number(v2);
    } else if (vm_obj_is_string(v1) && vm_obj_is_string(v2)) {
        return strcmp(vm_obj_get_string(v1)->buf, vm_obj_get_string(v2)->buf) < 0;
    } else {
        __builtin_trap();
        return false;
    }
}

static VM_INLINE bool vm_interp_value_le(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_get_number(v1) <= vm_obj_get_number(v2);
    } else if (vm_obj_is_string(v1) && vm_obj_is_string(v2)) {
        return strcmp(vm_obj_get_string(v1)->buf, vm_obj_get_string(v2)->buf) <= 0;
    } else {
        __builtin_trap();
        return false;
    }
}

static VM_INLINE vm_obj_t vm_interp_add(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) + vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad addition"));
    }
}

static VM_INLINE vm_obj_t vm_interp_sub(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) - vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad subtraction"));
    }
}

static VM_INLINE vm_obj_t vm_interp_mul(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) * vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad multiplication"));
    }
}

static VM_INLINE vm_obj_t vm_interp_div(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) / vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad division"));
    }
}

static VM_INLINE vm_obj_t vm_interp_idiv(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(floor(vm_obj_get_number(v1) / vm_obj_get_number(v2)));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad floor division"));
    }
}

static VM_INLINE vm_obj_t vm_interp_mod(vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(fmod(vm_obj_get_number(v1), vm_obj_get_number(v2)));
    } else {
        return vm_obj_of_error(vm_error_from_msg(vm_location_range_unknown, "bad modulo"));
    }
}

#if VM_UNALIGNED
#define vm_interp_push_num(size_) ({                      \
    size_t size = (size_);                                \
    if (len + size >= alloc) {                            \
        alloc += (len + size) * 2;                        \
        code = vm_realloc(code, sizeof(uint8_t) * alloc); \
    }                                                     \
    uint8_t *base = &code[len];                           \
    len += size;                                          \
    base;                                                 \
})
#define vm_interp_push(t, v) (*(t *)vm_interp_push_num(sizeof(t)) = (v))
#else
#define vm_interp_push(T, v_) ({                          \
    T val = (v_);                                         \
    if (len + sizeof(T) >= alloc) {                       \
        alloc += (len + sizeof(T)) * 2;                   \
        code = vm_realloc(code, sizeof(uint8_t) * alloc); \
    }                                                     \
    memcpy(&code[len], &val, sizeof(T));                  \
    len += sizeof(T);                                     \
})
#endif

#define vm_interp_push_op(v) vm_interp_push(void *, ptrs[v])

void *vm_interp_renumber_block(vm_t *vm, void **ptrs, vm_block_t *block) {
    size_t alloc = 32;
    size_t len = 0;
    uint8_t *code = vm_malloc(sizeof(uint8_t) * alloc);
    for (size_t i = 0; i < block->len; i++) {
        vm_instr_t instr = block->instrs[i];
        switch (instr.op) {
            case VM_IOP_NOP: {
                break;
            }
            case VM_IOP_MOVE: {
                if (instr.args[0].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MOVE_R);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_ADD: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_add(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_ADD_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_ADD_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_ADD_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_SUB: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_sub(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_SUB_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_SUB_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_SUB_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_MUL: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_mul(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_MUL_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MUL_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MUL_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_DIV: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_div(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_DIV_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_DIV_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_DIV_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_IDIV: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_idiv(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_IDIV_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_IDIV_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_IDIV_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_MOD: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_mod(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_MOD_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MOD_IR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MOD_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_TABLE_SET: {
                vm_interp_push_op(VM_OP_TABLE_SET);
                for (size_t i = 0; i < 3; i++) {
                    vm_interp_push(vm_interp_tag_t, instr.args[i].type);
                    if (instr.args[i].type == VM_ARG_LIT) {
                        vm_interp_push(vm_obj_t, instr.args[i].lit);
                    } else {
                        vm_interp_push(vm_interp_reg_t, instr.args[i].reg);
                    }
                }
                break;
            }
            case VM_IOP_TABLE_NEW: {
                vm_interp_push_op(VM_OP_TABLE_NEW);
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            case VM_IOP_TABLE_LEN: {
                vm_interp_push_op(VM_OP_TABLE_LEN);
                vm_interp_push(vm_interp_tag_t, instr.args[0].type);
                if (instr.args[0].type == VM_ARG_LIT) {
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                } else {
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                }
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            case VM_IOP_STD: {
                vm_interp_push_op(VM_OP_MOVE_I);
                vm_interp_push(vm_obj_t, vm->std);
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            default: {
                __builtin_trap();
            }
        }
    }
    vm_branch_t branch = block->branch;
    switch (branch.op) {
        case VM_BOP_FALL: {
            break;
        }
        case VM_BOP_JUMP: {
            vm_interp_push_op(VM_OP_JUMP);
            vm_interp_push(vm_block_t *, branch.targets[0]);
            break;
        }
        case VM_BOP_BB: {
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                if (!vm_obj_is_nil(v1) && (!vm_obj_is_boolean(v1) || vm_obj_get_boolean(v1))) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BB_R);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BLT: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_interp_value_lt(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BLT_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLT_IR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLT_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BLE: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_interp_value_le(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BLE_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLE_IR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLE_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BEQ: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_interp_value_eq(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BEQ_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BEQ_IR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BEQ_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, branch.targets[0]);
                vm_interp_push(vm_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_RET: {
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_RET_I);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else if (branch.args[0].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_RET_R);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_LOAD: {
            vm_interp_push_op(VM_OP_LOAD);
            vm_interp_push(vm_interp_tag_t, branch.args[0].type);
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push(vm_obj_t, branch.args[1].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, branch.targets[0]);
            break;
        }
        case VM_BOP_GET: {
            vm_interp_push_op(VM_OP_GET);
            vm_interp_push(vm_interp_tag_t, branch.args[0].type);
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push(vm_obj_t, branch.args[1].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, branch.targets[0]);
            break;
        }
        case VM_BOP_CALL: {
            vm_interp_push_op(VM_OP_CALL);
            for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
                if (branch.args[i].type == VM_ARG_LIT) {
                    vm_interp_push(vm_interp_tag_t, VM_ARG_LIT);
                    vm_interp_push(vm_obj_t, branch.args[i].lit);
                } else if (branch.args[i].type == VM_ARG_REG) {
                    vm_interp_push(vm_interp_tag_t, VM_ARG_REG);
                    vm_interp_push(vm_interp_reg_t, branch.args[i].reg);
                } else {
                    __builtin_trap();
                }
            }
            vm_interp_push(vm_interp_tag_t, VM_ARG_NONE);
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, branch.targets[0]);
            break;
        }
        default: {
            __builtin_trap();
        }
    }
    return code;
}

#if VM_UNALIGNED
#define vm_run_repl_read(T) ({ \
    T *ptr = (T *)code;        \
    code += sizeof(T);         \
    *ptr;                      \
})
#else
#define vm_run_repl_read(T) ({     \
    T ptr;                         \
    memcpy(&ptr, code, sizeof(T)); \
    code += sizeof(T);             \
    ptr;                           \
})
#endif

#define vm_run_repl_lit() vm_run_repl_read(vm_obj_t)
#define vm_run_repl_reg() (regs[vm_run_repl_read(vm_interp_reg_t)])

#if 0
#define vm_run_repl_arg() (                         \
    vm_run_repl_read(vm_interp_tag_t) == VM_ARG_LIT \
        ? (printf(":LIT\n"), vm_run_repl_lit())     \
        : (printf(":REG\n"), vm_run_repl_reg())     \
)
#else
#define vm_run_repl_arg() (                         \
    vm_run_repl_read(vm_interp_tag_t) == VM_ARG_LIT \
        ? vm_run_repl_lit()                         \
        : vm_run_repl_reg()                         \
)
#endif

#define vm_run_repl_out(value) (regs[vm_run_repl_read(vm_interp_reg_t)] = (value))

vm_obj_t vm_run_repl_inner(vm_t *vm, vm_block_t *block) {
    vm_obj_t *regs = vm->regs;
    static void *ptrs[VM_MAX_OP] = {
        [VM_OP_TABLE_SET] = &&VM_OP_TABLE_SET,
        [VM_OP_TABLE_NEW] = &&VM_OP_TABLE_NEW,
        [VM_OP_TABLE_LEN] = &&VM_OP_TABLE_LEN,
        [VM_OP_MOVE_I] = &&VM_OP_MOVE_I,
        [VM_OP_MOVE_R] = &&VM_OP_MOVE_R,
        [VM_OP_ADD_RI] = &&VM_OP_ADD_RI,
        [VM_OP_ADD_IR] = &&VM_OP_ADD_IR,
        [VM_OP_ADD_RR] = &&VM_OP_ADD_RR,
        [VM_OP_SUB_RI] = &&VM_OP_SUB_RI,
        [VM_OP_SUB_IR] = &&VM_OP_SUB_IR,
        [VM_OP_SUB_RR] = &&VM_OP_SUB_RR,
        [VM_OP_MUL_RI] = &&VM_OP_MUL_RI,
        [VM_OP_MUL_IR] = &&VM_OP_MUL_IR,
        [VM_OP_MUL_RR] = &&VM_OP_MUL_RR,
        [VM_OP_DIV_RI] = &&VM_OP_DIV_RI,
        [VM_OP_DIV_IR] = &&VM_OP_DIV_IR,
        [VM_OP_DIV_RR] = &&VM_OP_DIV_RR,
        [VM_OP_IDIV_RI] = &&VM_OP_IDIV_RI,
        [VM_OP_IDIV_IR] = &&VM_OP_IDIV_IR,
        [VM_OP_IDIV_RR] = &&VM_OP_IDIV_RR,
        [VM_OP_MOD_RI] = &&VM_OP_MOD_RI,
        [VM_OP_MOD_IR] = &&VM_OP_MOD_IR,
        [VM_OP_MOD_RR] = &&VM_OP_MOD_RR,
        [VM_OP_JUMP] = &&VM_OP_JUMP,
        [VM_OP_BB_R] = &&VM_OP_BB_R,
        [VM_OP_BLT_RI] = &&VM_OP_BLT_RI,
        [VM_OP_BLT_IR] = &&VM_OP_BLT_IR,
        [VM_OP_BLT_RR] = &&VM_OP_BLT_RR,
        [VM_OP_BLE_RI] = &&VM_OP_BLE_RI,
        [VM_OP_BLE_IR] = &&VM_OP_BLE_IR,
        [VM_OP_BLE_RR] = &&VM_OP_BLE_RR,
        [VM_OP_BEQ_RI] = &&VM_OP_BEQ_RI,
        [VM_OP_BEQ_IR] = &&VM_OP_BEQ_IR,
        [VM_OP_BEQ_RR] = &&VM_OP_BEQ_RR,
        [VM_OP_RET_I] = &&VM_OP_RET_I,
        [VM_OP_RET_R] = &&VM_OP_RET_R,
        [VM_OP_LOAD] = &&VM_OP_LOAD,
        [VM_OP_GET] = &&VM_OP_GET,
        [VM_OP_CALL] = &&VM_OP_CALL,
    };
    vm_obj_t *next_regs = &regs[VM_NREGS];
#if VM_DEBUG_BACKEND_BLOCKS
    {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_format_block(buf, block);
        printf("func%s\n", buf->buf);
    }
#endif
    goto new_block_no_print;

new_block:;

#if VM_DEBUG_BACKEND_BLOCKS
    {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_format_block(buf, block);
        printf("%s\n", buf->buf);
    }
#endif

new_block_no_print:;

    uint8_t *code = block->code;
    if (block->code == NULL) {
        code = vm_interp_renumber_block(vm, &ptrs[0], block);
        block->code = code;
    }

    goto *vm_run_repl_read(void *);

VM_OP_TABLE_SET:;
    VM_OPCODE_DEBUG(table_set) {
        vm_obj_t v1 = vm_run_repl_arg();
        vm_obj_t v2 = vm_run_repl_arg();
        vm_obj_t v3 = vm_run_repl_arg();
        if (!vm_obj_is_table(v1)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only set index on tables")));
        }
        vm_table_set(vm_obj_get_table(v1), v2, v3);
        vm_run_repl_jump();
    }
VM_OP_TABLE_NEW:;
    VM_OPCODE_DEBUG(table_new) {
        vm_run_repl_out(vm_obj_of_table(vm_table_new(vm)));
        vm_gc_run(vm, regs + VM_NREGS);
        vm_run_repl_jump();
    }
VM_OP_TABLE_LEN:;
    VM_OPCODE_DEBUG(table_len) {
        vm_obj_t v1 = vm_run_repl_arg();
        if (!vm_obj_is_table(v1)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only get length on tables")));
        }
        vm_run_repl_out(vm_obj_of_number(vm_obj_get_table(v1)->len));
        vm_run_repl_jump();
    }
VM_OP_MOVE_I:;
    VM_OPCODE_DEBUG(move_i) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_run_repl_out(v1);
        vm_run_repl_jump();
    }
VM_OP_MOVE_R:;
    VM_OPCODE_DEBUG(move_r) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_run_repl_out(v1);
        vm_run_repl_jump();
    }
VM_OP_ADD_RI:;
    VM_OPCODE_DEBUG(add_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_add(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_ADD_IR:;
    VM_OPCODE_DEBUG(add_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_add(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_ADD_RR:;
    VM_OPCODE_DEBUG(add_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_add(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_SUB_RI:;
    VM_OPCODE_DEBUG(sub_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_sub(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_SUB_IR:;
    VM_OPCODE_DEBUG(sub_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_sub(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_SUB_RR:;
    VM_OPCODE_DEBUG(sub_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_sub(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MUL_RI:;
    VM_OPCODE_DEBUG(mul_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_mul(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MUL_IR:;
    VM_OPCODE_DEBUG(mul_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mul(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MUL_RR:;
    VM_OPCODE_DEBUG(mul_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mul(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_DIV_RI:;
    VM_OPCODE_DEBUG(div_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_div(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_DIV_IR:;
    VM_OPCODE_DEBUG(div_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_div(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_DIV_RR:;
    VM_OPCODE_DEBUG(div_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_div(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_IDIV_RI:;
    VM_OPCODE_DEBUG(idiv_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_idiv(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_IDIV_IR:;
    VM_OPCODE_DEBUG(idiv_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_idiv(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_IDIV_RR:;
    VM_OPCODE_DEBUG(idiv_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_idiv(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MOD_RI:;
    VM_OPCODE_DEBUG(mod_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_mod(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MOD_IR:;
    VM_OPCODE_DEBUG(mod_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mod(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MOD_RR:;
    VM_OPCODE_DEBUG(mod_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mod(v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_JUMP:;
    VM_OPCODE_DEBUG(jump) {
        block = vm_run_repl_read(vm_block_t *);
        vm_backend_new_block();
    }
VM_OP_BB_R:;
    VM_OPCODE_DEBUG(bb_r) {
        vm_obj_t v1 = vm_run_repl_reg();
        if (!vm_obj_is_nil(v1) && (!vm_obj_is_boolean(v1) || vm_obj_get_boolean(v1))) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_RI:;
    VM_OPCODE_DEBUG(blt_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_IR:;
    VM_OPCODE_DEBUG(blt_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_RR:;
    VM_OPCODE_DEBUG(blt_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_RI:;
    VM_OPCODE_DEBUG(ble_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_IR:;
    VM_OPCODE_DEBUG(ble_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_RR:;
    VM_OPCODE_DEBUG(ble_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_RI:;
    VM_OPCODE_DEBUG(beq_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_IR:;
    VM_OPCODE_DEBUG(beq_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_RR:;
    VM_OPCODE_DEBUG(beq_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_RET_I:;
    VM_OPCODE_DEBUG(ret_i) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_backend_return(v1);
    }
VM_OP_RET_R:;
    VM_OPCODE_DEBUG(ret_r) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_backend_return(v1);
    }
VM_OP_LOAD:;
    VM_OPCODE_DEBUG(load) {
        vm_obj_t v1 = vm_run_repl_arg();
        vm_obj_t v2 = vm_run_repl_arg();
        vm_obj_t v3 = vm_obj_get_closure(v1)->values[(int32_t)vm_obj_get_number(v2)];
        vm_run_repl_out(v3);
        block = vm_run_repl_read(vm_block_t *);
        vm_backend_new_block();
    }
VM_OP_GET:;
    VM_OPCODE_DEBUG(get) {
        uint8_t *c0 = code;
        vm_obj_t v1 = vm_run_repl_arg();
        vm_obj_t v2 = vm_run_repl_arg();
        if (!vm_obj_is_table(v1)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only index tables")));
        }
        vm_table_pair_t pair = (vm_table_pair_t){
            .key = v2,
        };
        vm_table_get_pair(vm_obj_get_table(v1), &pair);
        vm_run_repl_out(pair.value);
        block = vm_run_repl_read(vm_block_t *);
        vm_backend_new_block();
    }
VM_OP_CALL:;
    VM_OPCODE_DEBUG(call) {
        vm_obj_t v1 = vm_run_repl_arg();
        if (vm_obj_is_closure(v1)) {
            next_regs[0] = v1;
            size_t j = 1;
        call_closure_next_arg:;
            uint8_t type = vm_run_repl_read(vm_interp_tag_t);
            switch (type) {
                case VM_ARG_NONE: {
                    goto call_closure_end;
                }
                case VM_ARG_REG: {
                    next_regs[j++] = vm_run_repl_reg();
                    goto call_closure_next_arg;
                }
                case VM_ARG_LIT: {
                    next_regs[j++] = vm_run_repl_lit();
                    goto call_closure_next_arg;
                }
                default: {
                    __builtin_unreachable();
                }
            }
        call_closure_end:;
            next_regs[j] = vm_obj_of_empty();
            vm_obj_t *last_regs = regs;
            vm->regs = next_regs;
            vm_obj_t got = vm_run_repl_inner(vm, vm_obj_get_closure(v1)->block);
            vm->regs = last_regs;
            if (vm_obj_is_error(got)) {
                vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(got))));
            }
            vm_run_repl_out(got);
            block = vm_run_repl_read(vm_block_t *);
            vm_backend_new_block();
        } else  if (vm_obj_is_ffi(v1)) {
            size_t j = 0;
        call_ffi_next_arg:;
            uint8_t type = vm_run_repl_read(vm_interp_tag_t);
            switch (type) {
                case VM_ARG_NONE: {
                    goto call_ffi_end;
                }
                case VM_ARG_REG: {
                    next_regs[j++] = vm_run_repl_reg();
                    goto call_ffi_next_arg;
                }
                case VM_ARG_LIT: {
                    next_regs[j++] = vm_run_repl_lit();
                    goto call_ffi_next_arg;
                }
                default: {
                    __builtin_unreachable();
                }
            }
        call_ffi_end:;
            next_regs[j] = vm_obj_of_empty();
            vm_obj_t *last_regs = regs;
            vm->regs = next_regs;
            vm_obj_get_ffi(v1)(vm, next_regs);
            vm->regs = last_regs;
            vm_obj_t got = next_regs[0];
            if (vm_obj_is_error(got)) {
                vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(got))));
            }
            vm_run_repl_out(next_regs[0]);
            vm_gc_run(vm, next_regs);
            block = vm_run_repl_read(vm_block_t *);
            vm_backend_new_block();
        } else {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only call functions")));
        }
        __builtin_unreachable();
    }
}

vm_obj_t vm_run_repl(vm_t *vm, vm_block_t *entry) {
    vm_blocks_t blocks = (vm_blocks_t) {
        .next = vm->blocks,
        .block = entry,
    };
    vm->blocks = &blocks;
    vm_obj_t ret = vm_run_repl_inner(vm, entry);
    vm->blocks = blocks.next;
    return ret;
}

vm_obj_t vm_run_main(vm_t *vm, vm_block_t *entry) {
    vm_obj_t val = vm_run_repl(vm, entry);
    if (vm_obj_is_error(val)) {
        vm_error_report(vm_obj_get_error(val), stderr);
    }
    return val;
}
