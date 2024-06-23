
#include "../vm.h"
#include "../obj.h"
#include "../ir.h"

#define COMBINE(x, y) ((x)*VM_TAG_MAX + (y))
#define CONCAT_2(x, y) x ## _ ## y
#define CONCAT(x, y) CONCAT_2(x, y)
#define CONCAT3_2(x, y, z) x ## _ ## y ## _ ## z
#define CONCAT3(x, y) CONCAT3_2(x, y)

#define VM_MATH_SWITCH 1

#if VM_MATH_SWITCH
#define VM_INLINE inline __attribute__((always_inline))
#else
#define VM_INLINE inline
#endif

#if 0
#define VM_OPCODE_DEBUG(s) printf("%s\n", #s);
#else
#define VM_OPCODE_DEBUG(s)
#endif

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

static VM_INLINE bool vm_interp_value_eq(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)==(y))
    #define OP_S(x, y) (strcmp((x), (y)) == 0)
    #define WRITE return
    #define NAME EQ
    #include "test.inc"
    __builtin_trap();
}

static VM_INLINE bool vm_interp_value_lt(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)<(y))
    #define OP_S(x, y) (strcmp((x), (y)) < 0)
    #define WRITE return
    #define NAME LT
    #include "test.inc"
    __builtin_trap();
}

static VM_INLINE bool vm_interp_value_le(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)<=(y))
    #define OP_S(x, y) (strcmp((x), (y)) <= 0)
    #define WRITE return
    #define NAME LE
    #include "test.inc"
    __builtin_trap();
}

static VM_INLINE vm_std_value_t vm_interp_add(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)+(y))
    #define WRITE return
    #define NAME ADD
    #include "binop.inc"
}

static VM_INLINE vm_std_value_t vm_interp_sub(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)-(y))
    #define WRITE return
    #define NAME SUB
    #include "binop.inc"
}

static VM_INLINE vm_std_value_t vm_interp_mul(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)*(y))
    #define WRITE return
    #define NAME MUL
    #include "binop.inc"
}

static VM_INLINE vm_std_value_t vm_interp_div(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)/(y))
    #define WRITE return
    #define NAME DIV
    #include "binop.inc"
}

static VM_INLINE vm_std_value_t vm_interp_idiv(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)/(y))
    #define OP_F(x, y) floor((x)/(y))
    #define WRITE return
    #define NAME IDIV
    #include "binop.inc"
}

static VM_INLINE vm_std_value_t vm_interp_mod(vm_std_value_t v1, vm_std_value_t v2) {
    #define OP(x, y) ((x)%(y))
    #define OP_F(x, y) fmod((x),(y))
    #define WRITE return
    #define NAME MOD
    #include "binop.inc"
}

#if VM_UNALIGNED
#define vm_interp_push_num(size_) ({ \
    size_t size = (size_); \
    if (len + size >= alloc) { \
        alloc += (len + size) * 2; \
        code = vm_realloc(code, sizeof(uint8_t) * alloc); \
    } \
    uint8_t *base = &code[len]; \
    len += size; \
    base; \
})
#define vm_interp_push(t, v) (*(t *)vm_interp_push_num(sizeof(t)) = (v))
#else
#define vm_interp_push(T, v_) ({\
    T val = (v_);\
    if (len + sizeof(T) >= alloc) { \
        alloc += (len + sizeof(T)) * 2; \
        code = vm_realloc(code, sizeof(uint8_t) * alloc); \
    } \
    memcpy(&code[len], &val, sizeof(T)); \
    len += sizeof(T);\
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
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MOVE_R);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_FUN) {
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, VM_TAG_FUN);
                    vm_interp_push(vm_value_t, (vm_value_t) {
                        .i32 = instr.args[0].func->id
                    });
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IOP_ADD: {
                if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_LIT) {
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_add(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_ADD_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_ADD_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_sub(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_SUB_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_SUB_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_mul(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_MUL_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MUL_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_div(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_DIV_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_DIV_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_idiv(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_IDIV_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_IDIV_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                    vm_std_value_t v1 = instr.args[0].lit;
                    vm_std_value_t v2 = instr.args[1].lit;
                    vm_std_value_t v3 = vm_interp_mod(v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_I);
                    vm_interp_push(vm_interp_tag_t, v3.tag);
                    vm_interp_push(vm_value_t, v3.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_REG && instr.args[1].type == VM_ARG_LIT) {
                    vm_interp_push_op(VM_OP_MOD_RI);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_tag_t, instr.args[1].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[1].lit.value);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_ARG_LIT && instr.args[1].type == VM_ARG_REG) {
                    vm_interp_push_op(VM_OP_MOD_IR);
                    vm_interp_push(vm_interp_tag_t, instr.args[0].lit.tag);
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
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
                        vm_interp_push(vm_interp_tag_t, instr.args[i].lit.tag);
                        vm_interp_push(vm_value_t, instr.args[i].lit.value);
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
                    vm_interp_push(vm_value_t, instr.args[0].lit.value);
                } else {
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                }
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            case VM_IOP_STD: {
                vm_interp_push_op(VM_OP_MOVE_I);
                vm_interp_push(vm_interp_tag_t, vm->std.tag);
                vm_interp_push(vm_value_t, vm->std.value);
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
            vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
            break;
        }
        case VM_BOP_BB: {
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_std_value_t v1 = branch.args[0].lit;
                if (v1.tag != VM_TAG_NIL && (v1.tag != VM_TAG_BOOL || v1.value.b)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
                }
            } else if (branch.args[0].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BB_R);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BLT: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = branch.args[0].lit;
                vm_std_value_t v2 = branch.args[1].lit;
                if (vm_interp_value_lt(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BLT_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_tag_t, branch.args[1].lit.tag);
                vm_interp_push(vm_value_t, branch.args[1].lit.value);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLT_IR);
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLT_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BLE: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = branch.args[0].lit;
                vm_std_value_t v2 = branch.args[1].lit;
                if (vm_interp_value_le(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BLE_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_tag_t, branch.args[1].lit.tag);
                vm_interp_push(vm_value_t, branch.args[1].lit.value);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLE_IR);
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BLE_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_BEQ: {
            if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_LIT) {
                vm_std_value_t v1 = branch.args[0].lit;
                vm_std_value_t v2 = branch.args[1].lit;
                if (vm_interp_value_eq(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
                }
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_BEQ_RI);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_tag_t, branch.args[1].lit.tag);
                vm_interp_push(vm_value_t, branch.args[1].lit.value);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_LIT && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BEQ_IR);
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else if (branch.args[0].type == VM_ARG_REG && branch.args[1].type == VM_ARG_REG) {
                vm_interp_push_op(VM_OP_BEQ_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
                vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[1]->id]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_BOP_RET: {
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_interp_push_op(VM_OP_RET_I);
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
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
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push(vm_interp_tag_t, branch.args[1].lit.tag);
                vm_interp_push(vm_value_t, branch.args[1].lit.value);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
            break;
        }
        case VM_BOP_GET: {
            vm_interp_push_op(VM_OP_GET);
            vm_interp_push(vm_interp_tag_t, branch.args[0].type);
            if (branch.args[0].type == VM_ARG_LIT) {
                vm_interp_push(vm_interp_tag_t, branch.args[0].lit.tag);
                vm_interp_push(vm_value_t, branch.args[0].lit.value);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_ARG_LIT) {
                vm_interp_push(vm_interp_tag_t, branch.args[1].lit.tag);
                vm_interp_push(vm_value_t, branch.args[1].lit.value);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
            break;
        }
        case VM_BOP_CALL: {
            vm_interp_push_op(VM_OP_CALL);
            for (size_t i = 0; branch.args[i].type != VM_ARG_NONE; i++) {
                if (branch.args[i].type == VM_ARG_LIT) {
                    vm_interp_push(vm_interp_tag_t, VM_ARG_LIT);
                    vm_interp_push(vm_interp_tag_t, branch.args[i].lit.tag);
                    vm_interp_push(vm_value_t, branch.args[i].lit.value);
                } else if (branch.args[i].type == VM_ARG_REG) {
                    vm_interp_push(vm_interp_tag_t, VM_ARG_REG);
                    vm_interp_push(vm_interp_reg_t, branch.args[i].reg);
                } else if (branch.args[i].type == VM_ARG_FUN) {
                    vm_interp_push(vm_interp_tag_t, VM_ARG_LIT);
                    vm_interp_push(vm_interp_tag_t, VM_TAG_FUN);
                    vm_interp_push(vm_value_t, ((vm_value_t) {.i32 = branch.args[i].func->id}));
                } else {
                    __builtin_trap();
                }
            }
            vm_interp_push(vm_interp_tag_t, VM_ARG_NONE);
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_block_t *, vm->blocks->blocks[branch.targets[0]->id]);
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
 T *ptr = (T*) code;\
 code += sizeof(T);\
 *ptr;\
})
#else
#define vm_run_repl_read(T) ({ \
 T ptr; \
 memcpy(&ptr, code, sizeof(T)); \
 code += sizeof(T); \
 ptr; \
})
#endif

#define vm_run_repl_lit() ({ \
    vm_tag_t tag = vm_run_repl_read(vm_interp_tag_t); \
    vm_value_t value = vm_run_repl_read(vm_value_t); \
    (vm_std_value_t) { \
        .tag = tag, \
        .value = value, \
    }; \
})
#define vm_run_repl_reg() (regs[vm_run_repl_read(vm_interp_reg_t)])

#if 0
#define vm_run_repl_arg() ( \
    vm_run_repl_read(vm_interp_tag_t) == VM_ARG_LIT \
        ? (printf(":LIT\n"), vm_run_repl_lit()) \
        : (printf(":REG\n"), vm_run_repl_reg()) \
    )
#else
#define vm_run_repl_arg() ( \
    vm_run_repl_read(vm_interp_tag_t) == VM_ARG_LIT \
        ? vm_run_repl_lit() \
        : vm_run_repl_reg() \
    )
#endif

#define vm_run_repl_out(value) (regs[vm_run_repl_read(vm_interp_reg_t)] = (value))

#define vm_run_repl_jump() goto *vm_run_repl_read(void *);

vm_std_value_t vm_run_repl(vm_t *vm, vm_block_t *block) {
    vm_std_value_t *regs = vm->regs;
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
    vm_std_value_t *next_regs = &regs[block->nregs];

    // {
    //     vm_io_buffer_t *buf = vm_io_buffer_new();
    //     vm_io_format_block(buf, block);
    //     printf("func%s\n", buf->buf);
    // }
    goto new_block_no_print;

new_block:;

    // {
    //     vm_io_buffer_t *buf = vm_io_buffer_new();
    //     vm_io_format_block(buf, block);
    //     printf("%s\n", buf->buf);
    // }

new_block_no_print:;

    uint8_t *code = block->code;
    if (block->code == NULL) {
        code = vm_interp_renumber_block(vm, &ptrs[0], block);
        block->code = code;
    }

    vm_run_repl_jump();

    VM_OP_TABLE_SET:; VM_OPCODE_DEBUG(table_set) {
        vm_std_value_t v1 = vm_run_repl_arg();
        vm_std_value_t v2 = vm_run_repl_arg();
        vm_std_value_t v3 = vm_run_repl_arg();
        if (v1.tag != VM_TAG_TAB) {
            return (vm_std_value_t) {
                .tag = VM_TAG_ERROR,
                .value.str = "can only index tables",
            };
        }
        vm_table_set(v1.value.table, v2.value, v3.value, v2.tag, v3.tag);
        vm_run_repl_jump();
    }
    VM_OP_TABLE_NEW:; VM_OPCODE_DEBUG(table_new) {
        vm_run_repl_out(((vm_std_value_t) {
            .tag = VM_TAG_TAB,
            .value.table = vm_table_new(),
        }));
        vm_run_repl_jump();
    }
    VM_OP_TABLE_LEN:; VM_OPCODE_DEBUG(table_len) {
        vm_std_value_t v1 = vm_run_repl_arg();
        if (v1.tag != VM_TAG_TAB) {
            return (vm_std_value_t) {
                .tag = VM_TAG_ERROR,
                .value.str = "can only get length on tables",
            };
        }
        vm_run_repl_out(VM_STD_VALUE_NUMBER(vm, v1.value.table->len));
        vm_run_repl_jump();
    }
    VM_OP_MOVE_I:; VM_OPCODE_DEBUG(move_i) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_run_repl_out(v1);
        vm_run_repl_jump();
    }
    VM_OP_MOVE_R:; VM_OPCODE_DEBUG(move_r) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_run_repl_out(v1);
        vm_run_repl_jump();
    }
    VM_OP_ADD_RI:; VM_OPCODE_DEBUG(add_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_add(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_ADD_IR:; VM_OPCODE_DEBUG(add_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_add(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_ADD_RR:; VM_OPCODE_DEBUG(add_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_add(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_SUB_RI:; VM_OPCODE_DEBUG(sub_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_sub(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_SUB_IR:; VM_OPCODE_DEBUG(sub_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_sub(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_SUB_RR:; VM_OPCODE_DEBUG(sub_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_sub(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MUL_RI:; VM_OPCODE_DEBUG(mul_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_mul(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MUL_IR:; VM_OPCODE_DEBUG(mul_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_mul(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MUL_RR:; VM_OPCODE_DEBUG(mul_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_mul(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_DIV_RI:; VM_OPCODE_DEBUG(div_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_div(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_DIV_IR:; VM_OPCODE_DEBUG(div_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_div(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_DIV_RR:; VM_OPCODE_DEBUG(div_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_div(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_IDIV_RI:; VM_OPCODE_DEBUG(idiv_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_idiv(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_IDIV_IR:; VM_OPCODE_DEBUG(idiv_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_idiv(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_IDIV_RR:; VM_OPCODE_DEBUG(idiv_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_idiv(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MOD_RI:; VM_OPCODE_DEBUG(mod_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        vm_std_value_t v3 = vm_interp_mod(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MOD_IR:; VM_OPCODE_DEBUG(mod_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_mod(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_MOD_RR:; VM_OPCODE_DEBUG(mod_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        vm_std_value_t v3 = vm_interp_mod(v1, v2);
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
    VM_OP_JUMP:; VM_OPCODE_DEBUG(jump) {
        block = vm_run_repl_read(vm_block_t *);
        goto new_block;
    }
    VM_OP_BB_R:; VM_OPCODE_DEBUG(bb_r) {
        vm_std_value_t v1 = vm_run_repl_reg();
        if (v1.tag != VM_TAG_NIL && (v1.tag != VM_TAG_BOOL || v1.value.b)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLT_RI:; VM_OPCODE_DEBUG(blt_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLT_IR:; VM_OPCODE_DEBUG(blt_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLT_RR:; VM_OPCODE_DEBUG(blt_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_interp_value_lt(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLE_RI:; VM_OPCODE_DEBUG(ble_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLE_IR:; VM_OPCODE_DEBUG(ble_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BLE_RR:; VM_OPCODE_DEBUG(ble_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_interp_value_le(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BEQ_RI:; VM_OPCODE_DEBUG(beq_ri) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_lit();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BEQ_IR:; VM_OPCODE_DEBUG(beq_ir) {
        vm_std_value_t v1 = vm_run_repl_lit();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_BEQ_RR:; VM_OPCODE_DEBUG(beq_rr) {
        vm_std_value_t v1 = vm_run_repl_reg();
        vm_std_value_t v2 = vm_run_repl_reg();
        if (vm_obj_eq(v1, v2)) {
            block = vm_run_repl_read(vm_block_t *);
        } else {
            vm_run_repl_read(vm_block_t *);
            block = vm_run_repl_read(vm_block_t *);
        }
        goto new_block;
    }
    VM_OP_RET_I:; VM_OPCODE_DEBUG(ret_i) {
        vm_std_value_t v1 = vm_run_repl_lit();
        return v1;
    }
    VM_OP_RET_R:; VM_OPCODE_DEBUG(ret_r) {
        vm_std_value_t v1 = vm_run_repl_reg();
        return v1;
    }
    VM_OP_LOAD:; VM_OPCODE_DEBUG(load) {
        vm_std_value_t v1 = vm_run_repl_arg();
        vm_std_value_t v2 = vm_run_repl_arg();
        vm_std_value_t v3;
        switch (v2.tag) {
            case VM_TAG_I8: {
                v3 = v1.value.closure[v2.value.i8];
                break;
            }
            case VM_TAG_I16: {
                v3 = v1.value.closure[v2.value.i16];
                break;
            }
            case VM_TAG_I32: {
                v3 = v1.value.closure[v2.value.i32];
                break;
            }
            case VM_TAG_I64: {
                v3 = v1.value.closure[v2.value.i64];
                break;
            }
            case VM_TAG_F32: {
                v3 = v1.value.closure[(int32_t) v2.value.f32];
                break;
            }
            case VM_TAG_F64: {
                v3 = v1.value.closure[(int32_t) v2.value.f64];
                break;
            }
            default: {
                __builtin_unreachable();
                break;
            }
        }
        vm_run_repl_out(v3);
        block = vm_run_repl_read(vm_block_t *);
        goto new_block;
    }
    VM_OP_GET:; VM_OPCODE_DEBUG(get) {
        uint8_t *c0 = code;
        vm_std_value_t v1 = vm_run_repl_arg();
        vm_std_value_t v2 = vm_run_repl_arg();
        vm_table_pair_t pair = (vm_table_pair_t) {
            .key_tag = v2.tag,
            .key_val = v2.value,
        };
        if (v1.tag != VM_TAG_TAB) {
            __builtin_trap();
            return (vm_std_value_t) {
                .tag = VM_TAG_ERROR,
                .value.str = "can only index tables",
            };
        }
        vm_table_get_pair(v1.value.table, &pair);
        vm_std_value_t v3 = (vm_std_value_t) {
            .tag = pair.val_tag,
            .value = pair.val_val,
        };
        vm_run_repl_out(v3);
        block = vm_run_repl_read(vm_block_t *);
        goto new_block;
    }
    VM_OP_CALL:; VM_OPCODE_DEBUG(call) {
        vm_std_value_t v1 = vm_run_repl_arg();
        switch (v1.tag) {
            case VM_TAG_CLOSURE: {
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
                next_regs[j].tag = VM_TAG_UNK;
                vm_std_value_t *last_regs = regs;
                vm->regs = next_regs;
                vm_std_value_t got = vm_run_repl(vm, vm->blocks->blocks[v1.value.closure[0].value.i32]);
                vm->regs = last_regs;
                if (got.tag == VM_TAG_ERROR) {
                    return got;
                }
                vm_run_repl_out(got);
                block = vm_run_repl_read(vm_block_t *);
                goto new_block;
            }
            case VM_TAG_FFI: {
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
                next_regs[j].tag = VM_TAG_UNK;
                vm_std_value_t *last_regs = regs;
                vm->regs = next_regs;
                v1.value.ffi(vm, next_regs);
                vm->regs = last_regs;
                vm_std_value_t got = next_regs[0];
                if (got.tag == VM_TAG_ERROR) {
                    return got;
                }
                vm_run_repl_out(next_regs[0]);
                block = vm_run_repl_read(vm_block_t *);
                goto new_block;
            }
            default: {
                return (vm_std_value_t) {
                    .tag = VM_TAG_ERROR,
                    .value.str = "can only call functions",
                };
            }
        }
        vm_run_repl_jump();
    }
}

vm_std_value_t vm_run_main(vm_t *vm, vm_block_t *entry) {
    vm_std_value_t val = vm_run_repl(vm, entry);
    if (val.tag == VM_TAG_ERROR) {
        fprintf(stderr, "error: %s\n", val.value.str);
    }
    return val;
}
