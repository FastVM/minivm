
#include <math.h>
#include <stddef.h>

#include "../lib.h"
#include "../gc.h"
#include "../ir.h"
#include "../obj.h"
#include "../tables.h"
#include "../math.h"
#include "../io.h"

#define VM_INLINE inline

#if VM_USE_SPALL_INSTR
#include "../../vendor/spall/auto.h"

#define VM_OPCODE_SPALL_BEGIN(s) ({                                           \
    char buf[64];                                                             \
    signed long n = snprintf(buf, 64, "line %" PRIi32 ": %s", block->id, #s); \
    spall_auto_buffer_begin(buf, n, "", 0);                                   \
})
#define VM_OPCODE_SPALL_END() \
    spall_auto_buffer_end()

#else

#define VM_OPCODE_SPALL_BEGIN(s)
#define VM_OPCODE_SPALL_END()

#endif

#if VM_DEBUG_BACKEND_OPCODES
#define VM_OPCODE_DEBUG(s)    \
    VM_OPCODE_SPALL_BEGIN(s); \
    printf("%s\n", #s);
#else
#define VM_OPCODE_DEBUG(s) VM_OPCODE_SPALL_BEGIN(s);
#endif


typedef ptrdiff_t vm_run_repl_label_t;
#define vm_run_repl_ref(name) (&&name - &&VM_RUN_REPL_BASE)
#define vm_run_repl_goto(ptr) goto *((ptr) + &&VM_RUN_REPL_BASE)

#define vm_backend_return(v) ({ \
    vm_obj_t return_ = (v);     \
    VM_OPCODE_SPALL_END();      \
    return return_;             \
})
#define vm_run_repl_jump() \
    VM_OPCODE_SPALL_END(); \
    vm_run_repl_goto(vm_run_repl_read(vm_run_repl_label_t))
#define vm_backend_new_block() \
    VM_OPCODE_SPALL_END();     \
    goto new_block

typedef uint8_t vm_interp_tag_t;
typedef uint32_t vm_interp_reg_t;

enum {
    VM_OP_TABLE_SET,
    VM_OP_TABLE_NEW,
    VM_OP_TABLE_LEN,

    VM_OP_MOVE_C,
    VM_OP_MOVE_R,

    VM_OP_ADD_RC,
    VM_OP_ADD_CR,
    VM_OP_ADD_RR,

    VM_OP_SUB_RC,
    VM_OP_SUB_CR,
    VM_OP_SUB_RR,

    VM_OP_MUL_RC,
    VM_OP_MUL_CR,
    VM_OP_MUL_RR,

    VM_OP_DIV_RC,
    VM_OP_DIV_CR,
    VM_OP_DIV_RR,

    VM_OP_IDIV_RC,
    VM_OP_IDIV_CR,
    VM_OP_IDIV_RR,

    VM_OP_MOD_RC,
    VM_OP_MOD_CR,
    VM_OP_MOD_RR,

    VM_OP_POW_RC,
    VM_OP_POW_CR,
    VM_OP_POW_RR,

    VM_OP_CONCAT_RC,
    VM_OP_CONCAT_CR,
    VM_OP_CONCAT_RR,

    VM_OP_JUMP,

    VM_OP_BB_R,

    VM_OP_BLT_RC,
    VM_OP_BLT_CR,
    VM_OP_BLT_RR,

    VM_OP_BLE_RC,
    VM_OP_BLE_CR,
    VM_OP_BLE_RR,

    VM_OP_BEQ_RC,
    VM_OP_BEQ_CR,
    VM_OP_BEQ_RR,

    VM_OP_RET_C,
    VM_OP_RET_R,

    VM_OP_LOAD,
    VM_OP_GET,
    VM_OP_CALL,

    VM_MAX_OP,
};

static VM_INLINE vm_obj_t vm_interp_add(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) + vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad addition"));
    }
}

static VM_INLINE vm_obj_t vm_interp_sub(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) - vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad subtraction"));
    }
}

static VM_INLINE vm_obj_t vm_interp_mul(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) * vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad multiplication"));
    }
}

static VM_INLINE vm_obj_t vm_interp_div(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(vm_obj_get_number(v1) / vm_obj_get_number(v2));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad division"));
    }
}

static VM_INLINE vm_obj_t vm_interp_idiv(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(floor(vm_obj_get_number(v1) / vm_obj_get_number(v2)));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad floor division"));
    }
}

static VM_INLINE vm_obj_t vm_interp_mod(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(fmod(vm_obj_get_number(v1), vm_obj_get_number(v2)));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad modulo"));
    }
}

static VM_INLINE vm_obj_t vm_interp_pow(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_number(v1) && vm_obj_is_number(v2)) {
        return vm_obj_of_number(pow(vm_obj_get_number(v1), vm_obj_get_number(v2)));
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad power"));
    }
}

static VM_INLINE vm_obj_t vm_interp_concat(vm_t *vm, vm_obj_t v1, vm_obj_t v2) {
    if (vm_obj_is_buffer(v1) && vm_obj_is_buffer(v2)) {
        vm_io_buffer_t *buf = vm_io_buffer_new();
        vm_io_buffer_object_tostring(buf, v1);
        vm_io_buffer_object_tostring(buf, v2);
        vm_obj_t ret = vm_obj_of_buffer(buf);
        vm_gc_add(vm, ret);
        return ret;
    } else {
        return vm_obj_of_error(vm_error_from_msg(VM_LOCATION_RANGE_UNKNOWN, "bad modulo"));
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

#define vm_interp_push_op(v) vm_interp_push(vm_run_repl_label_t, ptrs[v])

void *vm_interp_renumber_block(vm_t *vm, const vm_run_repl_label_t *ptrs, vm_ir_block_t *block) {
    size_t alloc = 32;
    size_t len = 0;
    uint8_t *code = vm_malloc(sizeof(uint8_t) * alloc);
    for (size_t i = 0; i < block->len; i++) {
        vm_ir_instr_t instr = block->instrs[i];
        switch (instr.op) {
            case VM_IR_INSTR_OPCODE_NOP: {
                break;
            }
            case VM_IR_INSTR_OPCODE_MOVE: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_MOVE_R);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_ADD: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_add(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_ADD_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_ADD_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_ADD_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_SUB: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_sub(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_SUB_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_SUB_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_SUB_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_MUL: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_mul(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_MUL_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_MUL_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_MUL_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_DIV: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_div(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_DIV_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_DIV_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_DIV_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_IDIV: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_idiv(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_IDIV_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_IDIV_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_IDIV_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_MOD: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_mod(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_MOD_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_MOD_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_MOD_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_POW: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_pow(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_POW_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_POW_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_POW_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_CONCAT: {
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_obj_t v1 = instr.args[0].lit;
                    vm_obj_t v2 = instr.args[1].lit;
                    vm_obj_t v3 = vm_interp_concat(vm, v1, v2);
                    vm_interp_push_op(VM_OP_MOVE_C);
                    vm_interp_push(vm_obj_t, v3);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push_op(VM_OP_CONCAT_RC);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_obj_t, instr.args[1].lit);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_LIT && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_CONCAT_CR);
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else if (instr.args[0].type == VM_IR_ARG_TYPE_REG && instr.args[1].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push_op(VM_OP_CONCAT_RR);
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                    vm_interp_push(vm_interp_reg_t, instr.args[1].reg);
                    vm_interp_push(vm_interp_reg_t, instr.out.reg);
                } else {
                    __builtin_trap();
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_TABLE_SET: {
                vm_interp_push_op(VM_OP_TABLE_SET);
                for (size_t i = 0; i < 3; i++) {
                    vm_interp_push(vm_interp_tag_t, instr.args[i].type);
                    if (instr.args[i].type == VM_IR_ARG_TYPE_LIT) {
                        vm_interp_push(vm_obj_t, instr.args[i].lit);
                    } else {
                        vm_interp_push(vm_interp_reg_t, instr.args[i].reg);
                    }
                }
                break;
            }
            case VM_IR_INSTR_OPCODE_TABLE_NEW: {
                vm_interp_push_op(VM_OP_TABLE_NEW);
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            case VM_IR_INSTR_OPCODE_TABLE_LEN: {
                vm_interp_push_op(VM_OP_TABLE_LEN);
                vm_interp_push(vm_interp_tag_t, instr.args[0].type);
                if (instr.args[0].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push(vm_obj_t, instr.args[0].lit);
                } else {
                    vm_interp_push(vm_interp_reg_t, instr.args[0].reg);
                }
                vm_interp_push(vm_interp_reg_t, instr.out.reg);
                break;
            }
            default: {
                __builtin_trap();
            }
        }
    }
    vm_ir_branch_t branch = block->branch;
    switch (branch.op) {
        case VM_IR_BRANCH_OPCODE_FALL: {
            break;
        }
        case VM_IR_BRANCH_OPCODE_JUMP: {
            vm_interp_push_op(VM_OP_JUMP);
            vm_interp_push(vm_ir_block_t *, branch.targets[0]);
            break;
        }
        case VM_IR_BRANCH_OPCODE_BOOL: {
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                if (!vm_obj_is_nil(v1) && (!vm_obj_is_boolean(v1) || vm_obj_get_boolean(v1))) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BB_R);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_LT: {
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_obj_unsafe_lt(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push_op(VM_OP_BLT_RC);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BLT_CR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BLT_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_LE: {
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_obj_unsafe_le(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push_op(VM_OP_BLE_RC);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BLE_CR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BLE_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_EQ: {
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_obj_t v1 = branch.args[0].lit;
                vm_obj_t v2 = branch.args[1].lit;
                if (vm_obj_unsafe_eq(v1, v2)) {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                } else {
                    vm_interp_push_op(VM_OP_JUMP);
                    vm_interp_push(vm_ir_block_t *, branch.targets[1]);
                }
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push_op(VM_OP_BEQ_RC);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_obj_t, branch.args[1].lit);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_LIT && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BEQ_CR);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG && branch.args[1].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_BEQ_RR);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
                vm_interp_push(vm_ir_block_t *, branch.targets[0]);
                vm_interp_push(vm_ir_block_t *, branch.targets[1]);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_IR_BRANCH_OPCODE_RETURN: {
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push_op(VM_OP_RET_C);
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else if (branch.args[0].type == VM_IR_ARG_TYPE_REG) {
                vm_interp_push_op(VM_OP_RET_R);
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            } else {
                __builtin_trap();
            }
            break;
        }
        case VM_IR_BRANCH_OPCODE_CAPTURE_LOAD: {
            vm_interp_push_op(VM_OP_LOAD);
            vm_interp_push(vm_interp_tag_t, branch.args[0].type);
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push(vm_obj_t, branch.args[1].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_ir_block_t *, branch.targets[0]);
            break;
        }
        case VM_IR_BRANCH_OPCODE_TABLE_GET: {
            vm_interp_push_op(VM_OP_GET);
            vm_interp_push(vm_interp_tag_t, branch.args[0].type);
            if (branch.args[0].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push(vm_obj_t, branch.args[0].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[0].reg);
            }
            vm_interp_push(vm_interp_tag_t, branch.args[1].type);
            if (branch.args[1].type == VM_IR_ARG_TYPE_LIT) {
                vm_interp_push(vm_obj_t, branch.args[1].lit);
            } else {
                vm_interp_push(vm_interp_reg_t, branch.args[1].reg);
            }
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_ir_block_t *, branch.targets[0]);
            break;
        }
        case VM_IR_BRANCH_OPCODE_CALL: {
            vm_interp_push_op(VM_OP_CALL);
            for (size_t i = 0; branch.args[i].type != VM_IR_ARG_TYPE_NONE; i++) {
                if (branch.args[i].type == VM_IR_ARG_TYPE_LIT) {
                    vm_interp_push(vm_interp_tag_t, VM_IR_ARG_TYPE_LIT);
                    vm_interp_push(vm_obj_t, branch.args[i].lit);
                } else if (branch.args[i].type == VM_IR_ARG_TYPE_REG) {
                    vm_interp_push(vm_interp_tag_t, VM_IR_ARG_TYPE_REG);
                    vm_interp_push(vm_interp_reg_t, branch.args[i].reg);
                } else {
                    __builtin_trap();
                }
            }
            vm_interp_push(vm_interp_tag_t, VM_IR_ARG_TYPE_NONE);
            vm_interp_push(vm_interp_reg_t, branch.out.reg);
            vm_interp_push(vm_ir_block_t *, branch.targets[0]);
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
    vm_run_repl_read(vm_interp_tag_t) == VM_IR_ARG_TYPE_LIT \
        ? (printf(":LIT\n"), vm_run_repl_lit())     \
        : (printf(":REG\n"), vm_run_repl_reg())     \
)
#else
#define vm_run_repl_arg() (                         \
    vm_run_repl_read(vm_interp_tag_t) == VM_IR_ARG_TYPE_LIT \
        ? vm_run_repl_lit()                         \
        : vm_run_repl_reg()                         \
)
#endif

#define vm_run_repl_out(value) (regs[vm_run_repl_read(vm_interp_reg_t)] = (value))

size_t vm_interp_renumber_blocks(vm_t *vm, const vm_run_repl_label_t *ptrs, vm_ir_block_t *block) {
    size_t nregs = block->nregs;
    if (* (const vm_run_repl_label_t *) block->code == 0) {
        block->code = vm_interp_renumber_block(vm, &ptrs[0], block);
        for (size_t i = 0; i < 2; i++) {
            vm_ir_block_t *target = block->branch.targets[i];
            if (target != NULL) {
                size_t next_nregs = vm_interp_renumber_blocks(vm, ptrs, target);
                if (next_nregs > nregs) {
                    nregs = next_nregs;
                }
            }
        }
        // printf("bb%zu: %zu regs\n", (size_t) block->id, (size_t) block->nregs);
        block->nregs = nregs;
    }
    return nregs;
}


vm_obj_t vm_run_repl_inner(vm_t *vm, vm_ir_block_t *block) {
    vm_obj_t *regs = vm->regs;
    static const vm_run_repl_label_t ptrs[VM_MAX_OP] = {
        [VM_OP_TABLE_SET] = vm_run_repl_ref(VM_OP_TABLE_SET),
        [VM_OP_TABLE_NEW] = vm_run_repl_ref(VM_OP_TABLE_NEW),
        [VM_OP_TABLE_LEN] = vm_run_repl_ref(VM_OP_TABLE_LEN),
        [VM_OP_MOVE_C] = vm_run_repl_ref(VM_OP_MOVE_C),
        [VM_OP_MOVE_R] = vm_run_repl_ref(VM_OP_MOVE_R),
        [VM_OP_ADD_RC] = vm_run_repl_ref(VM_OP_ADD_RC),
        [VM_OP_ADD_CR] = vm_run_repl_ref(VM_OP_ADD_CR),
        [VM_OP_ADD_RR] = vm_run_repl_ref(VM_OP_ADD_RR),
        [VM_OP_SUB_RC] = vm_run_repl_ref(VM_OP_SUB_RC),
        [VM_OP_SUB_CR] = vm_run_repl_ref(VM_OP_SUB_CR),
        [VM_OP_SUB_RR] = vm_run_repl_ref(VM_OP_SUB_RR),
        [VM_OP_MUL_RC] = vm_run_repl_ref(VM_OP_MUL_RC),
        [VM_OP_MUL_CR] = vm_run_repl_ref(VM_OP_MUL_CR),
        [VM_OP_MUL_RR] = vm_run_repl_ref(VM_OP_MUL_RR),
        [VM_OP_DIV_RC] = vm_run_repl_ref(VM_OP_DIV_RC),
        [VM_OP_DIV_CR] = vm_run_repl_ref(VM_OP_DIV_CR),
        [VM_OP_DIV_RR] = vm_run_repl_ref(VM_OP_DIV_RR),
        [VM_OP_IDIV_RC] = vm_run_repl_ref(VM_OP_IDIV_RC),
        [VM_OP_IDIV_CR] = vm_run_repl_ref(VM_OP_IDIV_CR),
        [VM_OP_IDIV_RR] = vm_run_repl_ref(VM_OP_IDIV_RR),
        [VM_OP_MOD_RC] = vm_run_repl_ref(VM_OP_MOD_RC),
        [VM_OP_MOD_CR] = vm_run_repl_ref(VM_OP_MOD_CR),
        [VM_OP_MOD_RR] = vm_run_repl_ref(VM_OP_MOD_RR),
        [VM_OP_POW_RC] = vm_run_repl_ref(VM_OP_POW_RC),
        [VM_OP_POW_CR] = vm_run_repl_ref(VM_OP_POW_CR),
        [VM_OP_POW_RR] = vm_run_repl_ref(VM_OP_POW_RR),
        [VM_OP_CONCAT_RC] = vm_run_repl_ref(VM_OP_CONCAT_RC),
        [VM_OP_CONCAT_CR] = vm_run_repl_ref(VM_OP_CONCAT_CR),
        [VM_OP_CONCAT_RR] = vm_run_repl_ref(VM_OP_CONCAT_RR),
        [VM_OP_JUMP] = vm_run_repl_ref(VM_OP_JUMP),
        [VM_OP_BB_R] = vm_run_repl_ref(VM_OP_BB_R),
        [VM_OP_BLT_RC] = vm_run_repl_ref(VM_OP_BLT_RC),
        [VM_OP_BLT_CR] = vm_run_repl_ref(VM_OP_BLT_CR),
        [VM_OP_BLT_RR] = vm_run_repl_ref(VM_OP_BLT_RR),
        [VM_OP_BLE_RC] = vm_run_repl_ref(VM_OP_BLE_RC),
        [VM_OP_BLE_CR] = vm_run_repl_ref(VM_OP_BLE_CR),
        [VM_OP_BLE_RR] = vm_run_repl_ref(VM_OP_BLE_RR),
        [VM_OP_BEQ_RC] = vm_run_repl_ref(VM_OP_BEQ_RC),
        [VM_OP_BEQ_CR] = vm_run_repl_ref(VM_OP_BEQ_CR),
        [VM_OP_BEQ_RR] = vm_run_repl_ref(VM_OP_BEQ_RR),
        [VM_OP_RET_C] = vm_run_repl_ref(VM_OP_RET_C),
        [VM_OP_RET_R] = vm_run_repl_ref(VM_OP_RET_R),
        [VM_OP_LOAD] = vm_run_repl_ref(VM_OP_LOAD),
        [VM_OP_GET] = vm_run_repl_ref(VM_OP_GET),
        [VM_OP_CALL] = vm_run_repl_ref(VM_OP_CALL),
    };

    vm_obj_t *next_regs = &regs[block->nregs];
    
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
    const uint8_t *code = (const void *) block->code;

    vm_run_repl_goto(vm_run_repl_read(vm_run_repl_label_t));

VM_RUN_REPL_BASE:;
    size_t nregs = vm_interp_renumber_blocks(vm, ptrs, block);
    code = (const void *) block->code;
    next_regs = &regs[block->nregs];
    vm_run_repl_goto(vm_run_repl_read(vm_run_repl_label_t));

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
        vm_gc_run(vm, next_regs);
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
VM_OP_MOVE_C:;
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
VM_OP_ADD_RC:;
    VM_OPCODE_DEBUG(add_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_add(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_ADD_CR:;
    VM_OPCODE_DEBUG(add_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_add(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_add(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_SUB_RC:;
    VM_OPCODE_DEBUG(sub_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_sub(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_SUB_CR:;
    VM_OPCODE_DEBUG(sub_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_sub(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_sub(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MUL_RC:;
    VM_OPCODE_DEBUG(mul_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_mul(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MUL_CR:;
    VM_OPCODE_DEBUG(mul_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mul(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_mul(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_DIV_RC:;
    VM_OPCODE_DEBUG(div_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_div(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_DIV_CR:;
    VM_OPCODE_DEBUG(div_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_div(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_div(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_IDIV_RC:;
    VM_OPCODE_DEBUG(idiv_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_idiv(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_IDIV_CR:;
    VM_OPCODE_DEBUG(idiv_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_idiv(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_idiv(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MOD_RC:;
    VM_OPCODE_DEBUG(mod_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_mod(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_MOD_CR:;
    VM_OPCODE_DEBUG(mod_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_mod(vm, v1, v2);
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
        vm_obj_t v3 = vm_interp_mod(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_POW_RC:;
    VM_OPCODE_DEBUG(mod_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_pow(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_POW_CR:;
    VM_OPCODE_DEBUG(mod_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_pow(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_POW_RR:;
    VM_OPCODE_DEBUG(mod_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_pow(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_CONCAT_RC:;
    VM_OPCODE_DEBUG(mod_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        vm_obj_t v3 = vm_interp_concat(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_CONCAT_CR:;
    VM_OPCODE_DEBUG(mod_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_concat(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_CONCAT_RR:;
    VM_OPCODE_DEBUG(mod_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        vm_obj_t v3 = vm_interp_concat(vm, v1, v2);
        if (vm_obj_is_error(v3)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(v3))));
        }
        vm_run_repl_out(v3);
        vm_run_repl_jump();
    }
VM_OP_JUMP:;
    VM_OPCODE_DEBUG(jump) {
        block = vm_run_repl_read(vm_ir_block_t *);
        vm_backend_new_block();
    }
VM_OP_BB_R:;
    VM_OPCODE_DEBUG(bb_r) {
        vm_obj_t v1 = vm_run_repl_reg();
        if (!vm_obj_is_nil(v1) && (!vm_obj_is_boolean(v1) || vm_obj_get_boolean(v1))) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_RC:;
    VM_OPCODE_DEBUG(blt_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_obj_unsafe_lt(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_CR:;
    VM_OPCODE_DEBUG(blt_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_lt(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLT_RR:;
    VM_OPCODE_DEBUG(blt_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_lt(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_RC:;
    VM_OPCODE_DEBUG(ble_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_obj_unsafe_le(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_CR:;
    VM_OPCODE_DEBUG(ble_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_le(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BLE_RR:;
    VM_OPCODE_DEBUG(ble_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_le(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_RC:;
    VM_OPCODE_DEBUG(beq_ri) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_lit();
        if (vm_obj_unsafe_eq(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_CR:;
    VM_OPCODE_DEBUG(beq_ir) {
        vm_obj_t v1 = vm_run_repl_lit();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_eq(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_BEQ_RR:;
    VM_OPCODE_DEBUG(beq_rr) {
        vm_obj_t v1 = vm_run_repl_reg();
        vm_obj_t v2 = vm_run_repl_reg();
        if (vm_obj_unsafe_eq(v1, v2)) {
            block = vm_run_repl_read(vm_ir_block_t *);
        } else {
            vm_run_repl_read(vm_ir_block_t *);
            block = vm_run_repl_read(vm_ir_block_t *);
        }
        vm_backend_new_block();
    }
VM_OP_RET_C:;
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
        block = vm_run_repl_read(vm_ir_block_t *);
        vm_backend_new_block();
    }
VM_OP_GET:;
    VM_OPCODE_DEBUG(get) {
        vm_obj_t v1 = vm_run_repl_arg();
        vm_obj_t v2 = vm_run_repl_arg();
        if (!vm_obj_is_table(v1)) {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only index tables")));
        }
        vm_obj_t v3 = vm_table_get(vm_obj_get_table(v1), v2);
        vm_run_repl_out(v3);
        block = vm_run_repl_read(vm_ir_block_t *);
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
                case VM_IR_ARG_TYPE_NONE: {
                    goto call_closure_end;
                }
                case VM_IR_ARG_TYPE_REG: {
                    next_regs[j++] = vm_run_repl_reg();
                    goto call_closure_next_arg;
                }
                case VM_IR_ARG_TYPE_LIT: {
                    next_regs[j++] = vm_run_repl_lit();
                    goto call_closure_next_arg;
                }
                default: {
                    __builtin_unreachable();
                }
            }
        call_closure_end:;
            next_regs[j] = vm_obj_of_nil();
            vm_obj_t *last_regs = regs;
            vm->regs = next_regs;
            vm_obj_t got = vm_run_repl_inner(vm, vm_obj_get_closure(v1)->block);
            vm->regs = last_regs;
            if (vm_obj_is_error(got)) {
                vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(got))));
            }
            vm_run_repl_out(got);
            block = vm_run_repl_read(vm_ir_block_t *);
            vm_backend_new_block();
        } else if (vm_obj_is_ffi(v1)) {
            size_t j = 0;
        call_ffi_next_arg:;
            uint8_t type = vm_run_repl_read(vm_interp_tag_t);
            switch (type) {
                case VM_IR_ARG_TYPE_NONE: {
                    goto call_ffi_end;
                }
                case VM_IR_ARG_TYPE_REG: {
                    next_regs[j++] = vm_run_repl_reg();
                    goto call_ffi_next_arg;
                }
                case VM_IR_ARG_TYPE_LIT: {
                    next_regs[j++] = vm_run_repl_lit();
                    goto call_ffi_next_arg;
                }
                default: {
                    __builtin_unreachable();
                }
            }
        call_ffi_end:;
            vm_obj_t *last_regs = regs;
            vm->regs = next_regs;
            vm_obj_get_ffi(v1)(vm, j, next_regs);
            vm->regs = last_regs;
            vm_obj_t got = next_regs[0];
            if (vm_obj_is_error(got)) {
                vm_backend_return(vm_obj_of_error(vm_error_from_error(block->range, vm_obj_get_error(got))));
            }
            vm_run_repl_out(next_regs[0]);
            vm_gc_run(vm, next_regs);
            block = vm_run_repl_read(vm_ir_block_t *);
            vm_backend_new_block();
        } else {
            vm_backend_return(vm_obj_of_error(vm_error_from_msg(block->range, "can only call functions")));
        }
        __builtin_unreachable();
    }
}

vm_obj_t vm_run_repl(vm_t *vm, vm_ir_block_t *entry) {
    vm_ir_blocks_t blocks = (vm_ir_blocks_t){
        .next = vm->blocks,
        .block = entry,
    };
    vm->blocks = &blocks;
    vm_obj_t ret = vm_run_repl_inner(vm, entry);
    vm->blocks = blocks.next;
    return ret;
}

vm_obj_t vm_run_main(vm_t *vm, vm_ir_block_t *entry) {
    vm_obj_t val = vm_run_repl(vm, entry);
    if (vm_obj_is_error(val)) {
        vm_error_report(vm_obj_get_error(val), stderr);
    }
    return val;
}
