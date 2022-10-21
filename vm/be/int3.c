#include "./int3.h"
#include "./value.h"
#include "../tag.h"
void vm_run(vm_state_t *state, vm_block_t *block) {
    vm_opcode_t *restrict ip = vm_run_comp(state, vm_rblock_new(block, vm_rblock_regs_empty()));
    vm_value_t *restrict locals = state->locals;
    vm_opcode_t **restrict ips = state->ips;
    redo:; switch ((ip++)->reg) {
    case VM_OPCODE_ADD_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_I8_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I8_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I8_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I8_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I8_REG_PTR_PTR: {
        int8_t a0 = locals[ip[0].reg].i8;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_I8_CONST_PTR_PTR: {
        int8_t a0 = ip[0].i8;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I8_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_REG_REG_PTR_PTR: {
        int8_t a0 = locals[ip[0].reg].i8;
        int8_t a1 = ip[1].i8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I8_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_REG_CONST_PTR_PTR: {
        int8_t a0 = locals[ip[0].reg].i8;
        int8_t a1 = ip[1].i8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I8_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_CONST_REG_PTR_PTR: {
        int8_t a0 = ip[0].i8;
        int8_t a1 = ip[1].i8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I8_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I8_CONST_CONST_PTR_PTR: {
        int8_t a0 = ip[0].i8;
        int8_t a1 = ip[1].i8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I8_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I8_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I8_REG_REG_PTR_PTR: {
        int8_t a0 = locals[ip[0].reg].i8;
        int8_t a1 = ip[1].i8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I8_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I8_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I8_REG_CONST_PTR_PTR: {
        int8_t a0 = locals[ip[0].reg].i8;
        int8_t a1 = ip[1].i8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I8_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I8_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I8_CONST_REG_PTR_PTR: {
        int8_t a0 = ip[0].i8;
        int8_t a1 = ip[1].i8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I8_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I8_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I8_CONST_CONST_PTR_PTR: {
        int8_t a0 = ip[0].i8;
        int8_t a1 = ip[1].i8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_I8_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_I8_CONST: {
        int8_t a0 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_I8_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_I8_CONST: {
        int8_t a0 = (ip++)->i8;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_I8_VOID: {
        locals[(ip++)->reg].i8 = (int8_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_I8_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i8 = (int8_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_I8_CONST: {
        int8_t a0 = (ip++)->i8;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i8 = (int8_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I8_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I8_CONST: {
        int8_t a0 = (ip++)->i8;
        locals[(ip++)->reg].i8 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I8_REG_REG: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I8_REG_CONST: {
        int8_t a0 = locals[(ip++)->reg].i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I8_CONST_REG: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = locals[(ip++)->reg].i8;
        locals[(ip++)->reg].i8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I8_CONST_CONST: {
        int8_t a0 = (ip++)->i8;
        int8_t a1 = (ip++)->i8;
        locals[(ip++)->reg].i8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_I16_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I16_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I16_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I16_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I16_REG_PTR_PTR: {
        int16_t a0 = locals[ip[0].reg].i16;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_I16_CONST_PTR_PTR: {
        int16_t a0 = ip[0].i16;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I16_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_REG_REG_PTR_PTR: {
        int16_t a0 = locals[ip[0].reg].i16;
        int16_t a1 = ip[1].i16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I16_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_REG_CONST_PTR_PTR: {
        int16_t a0 = locals[ip[0].reg].i16;
        int16_t a1 = ip[1].i16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I16_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_CONST_REG_PTR_PTR: {
        int16_t a0 = ip[0].i16;
        int16_t a1 = ip[1].i16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I16_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I16_CONST_CONST_PTR_PTR: {
        int16_t a0 = ip[0].i16;
        int16_t a1 = ip[1].i16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I16_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I16_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I16_REG_REG_PTR_PTR: {
        int16_t a0 = locals[ip[0].reg].i16;
        int16_t a1 = ip[1].i16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I16_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I16_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I16_REG_CONST_PTR_PTR: {
        int16_t a0 = locals[ip[0].reg].i16;
        int16_t a1 = ip[1].i16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I16_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I16_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I16_CONST_REG_PTR_PTR: {
        int16_t a0 = ip[0].i16;
        int16_t a1 = ip[1].i16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I16_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I16_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I16_CONST_CONST_PTR_PTR: {
        int16_t a0 = ip[0].i16;
        int16_t a1 = ip[1].i16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_I16_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_I16_CONST: {
        int16_t a0 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_I16_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_I16_CONST: {
        int16_t a0 = (ip++)->i16;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_I16_VOID: {
        locals[(ip++)->reg].i16 = (int16_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_I16_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i16 = (int16_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_I16_CONST: {
        int16_t a0 = (ip++)->i16;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i16 = (int16_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I16_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I16_CONST: {
        int16_t a0 = (ip++)->i16;
        locals[(ip++)->reg].i16 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I16_REG_REG: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I16_REG_CONST: {
        int16_t a0 = locals[(ip++)->reg].i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I16_CONST_REG: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = locals[(ip++)->reg].i16;
        locals[(ip++)->reg].i16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I16_CONST_CONST: {
        int16_t a0 = (ip++)->i16;
        int16_t a1 = (ip++)->i16;
        locals[(ip++)->reg].i16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_I32_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I32_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I32_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I32_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I32_REG_PTR_PTR: {
        int32_t a0 = locals[ip[0].reg].i32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_I32_CONST_PTR_PTR: {
        int32_t a0 = ip[0].i32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_REG_REG_PTR_PTR: {
        int32_t a0 = locals[ip[0].reg].i32;
        int32_t a1 = ip[1].i32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_REG_CONST_PTR_PTR: {
        int32_t a0 = locals[ip[0].reg].i32;
        int32_t a1 = ip[1].i32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_CONST_REG_PTR_PTR: {
        int32_t a0 = ip[0].i32;
        int32_t a1 = ip[1].i32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I32_CONST_CONST_PTR_PTR: {
        int32_t a0 = ip[0].i32;
        int32_t a1 = ip[1].i32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I32_REG_REG_PTR_PTR: {
        int32_t a0 = locals[ip[0].reg].i32;
        int32_t a1 = ip[1].i32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I32_REG_CONST_PTR_PTR: {
        int32_t a0 = locals[ip[0].reg].i32;
        int32_t a1 = ip[1].i32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I32_CONST_REG_PTR_PTR: {
        int32_t a0 = ip[0].i32;
        int32_t a1 = ip[1].i32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I32_CONST_CONST_PTR_PTR: {
        int32_t a0 = ip[0].i32;
        int32_t a1 = ip[1].i32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_I32_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_I32_CONST: {
        int32_t a0 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_I32_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_I32_CONST: {
        int32_t a0 = (ip++)->i32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_I32_VOID: {
        locals[(ip++)->reg].i32 = (int32_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_I32_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i32 = (int32_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_I32_CONST: {
        int32_t a0 = (ip++)->i32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i32 = (int32_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I32_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I32_CONST: {
        int32_t a0 = (ip++)->i32;
        locals[(ip++)->reg].i32 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I32_REG_REG: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I32_REG_CONST: {
        int32_t a0 = locals[(ip++)->reg].i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I32_CONST_REG: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = locals[(ip++)->reg].i32;
        locals[(ip++)->reg].i32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I32_CONST_CONST: {
        int32_t a0 = (ip++)->i32;
        int32_t a1 = (ip++)->i32;
        locals[(ip++)->reg].i32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_I64_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I64_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I64_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_I64_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_I64_REG_PTR_PTR: {
        int64_t a0 = locals[ip[0].reg].i64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_I64_CONST_PTR_PTR: {
        int64_t a0 = ip[0].i64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_REG_REG_PTR_PTR: {
        int64_t a0 = locals[ip[0].reg].i64;
        int64_t a1 = ip[1].i64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_REG_CONST_PTR_PTR: {
        int64_t a0 = locals[ip[0].reg].i64;
        int64_t a1 = ip[1].i64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_CONST_REG_PTR_PTR: {
        int64_t a0 = ip[0].i64;
        int64_t a1 = ip[1].i64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_I64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_I64_CONST_CONST_PTR_PTR: {
        int64_t a0 = ip[0].i64;
        int64_t a1 = ip[1].i64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I64_REG_REG_PTR_PTR: {
        int64_t a0 = locals[ip[0].reg].i64;
        int64_t a1 = ip[1].i64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I64_REG_CONST_PTR_PTR: {
        int64_t a0 = locals[ip[0].reg].i64;
        int64_t a1 = ip[1].i64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I64_CONST_REG_PTR_PTR: {
        int64_t a0 = ip[0].i64;
        int64_t a1 = ip[1].i64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_I64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_I64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_I64_CONST_CONST_PTR_PTR: {
        int64_t a0 = ip[0].i64;
        int64_t a1 = ip[1].i64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_I64_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_I64_CONST: {
        int64_t a0 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_I64_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_I64_CONST: {
        int64_t a0 = (ip++)->i64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_I64_VOID: {
        locals[(ip++)->reg].i64 = (int64_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_I64_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i64 = (int64_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_I64_CONST: {
        int64_t a0 = (ip++)->i64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].i64 = (int64_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I64_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_I64_CONST: {
        int64_t a0 = (ip++)->i64;
        locals[(ip++)->reg].i64 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I64_REG_REG: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I64_REG_CONST: {
        int64_t a0 = locals[(ip++)->reg].i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I64_CONST_REG: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = locals[(ip++)->reg].i64;
        locals[(ip++)->reg].i64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_I64_CONST_CONST: {
        int64_t a0 = (ip++)->i64;
        int64_t a1 = (ip++)->i64;
        locals[(ip++)->reg].i64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_U8_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U8_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U8_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U8_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U8_REG_PTR_PTR: {
        uint8_t a0 = locals[ip[0].reg].u8;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_U8_CONST_PTR_PTR: {
        uint8_t a0 = ip[0].u8;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U8_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_REG_REG_PTR_PTR: {
        uint8_t a0 = locals[ip[0].reg].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U8_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_REG_CONST_PTR_PTR: {
        uint8_t a0 = locals[ip[0].reg].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U8_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_CONST_REG_PTR_PTR: {
        uint8_t a0 = ip[0].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U8_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U8_CONST_CONST_PTR_PTR: {
        uint8_t a0 = ip[0].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U8_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U8_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U8_REG_REG_PTR_PTR: {
        uint8_t a0 = locals[ip[0].reg].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U8_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U8_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U8_REG_CONST_PTR_PTR: {
        uint8_t a0 = locals[ip[0].reg].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U8_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U8_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U8_CONST_REG_PTR_PTR: {
        uint8_t a0 = ip[0].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U8_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U8_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U8_CONST_CONST_PTR_PTR: {
        uint8_t a0 = ip[0].u8;
        uint8_t a1 = ip[1].u8;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_U8_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_U8_CONST: {
        uint8_t a0 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_U8_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_U8_CONST: {
        uint8_t a0 = (ip++)->u8;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_U8_VOID: {
        locals[(ip++)->reg].u8 = (uint8_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_U8_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u8 = (uint8_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_U8_CONST: {
        uint8_t a0 = (ip++)->u8;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u8 = (uint8_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U8_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U8_CONST: {
        uint8_t a0 = (ip++)->u8;
        locals[(ip++)->reg].u8 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U8_REG_REG: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U8_REG_CONST: {
        uint8_t a0 = locals[(ip++)->reg].u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U8_CONST_REG: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = locals[(ip++)->reg].u8;
        locals[(ip++)->reg].u8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U8_CONST_CONST: {
        uint8_t a0 = (ip++)->u8;
        uint8_t a1 = (ip++)->u8;
        locals[(ip++)->reg].u8 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_U16_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U16_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U16_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U16_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U16_REG_PTR_PTR: {
        uint16_t a0 = locals[ip[0].reg].u16;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_U16_CONST_PTR_PTR: {
        uint16_t a0 = ip[0].u16;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U16_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_REG_REG_PTR_PTR: {
        uint16_t a0 = locals[ip[0].reg].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U16_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_REG_CONST_PTR_PTR: {
        uint16_t a0 = locals[ip[0].reg].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U16_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_CONST_REG_PTR_PTR: {
        uint16_t a0 = ip[0].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U16_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U16_CONST_CONST_PTR_PTR: {
        uint16_t a0 = ip[0].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U16_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U16_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U16_REG_REG_PTR_PTR: {
        uint16_t a0 = locals[ip[0].reg].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U16_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U16_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U16_REG_CONST_PTR_PTR: {
        uint16_t a0 = locals[ip[0].reg].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U16_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U16_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U16_CONST_REG_PTR_PTR: {
        uint16_t a0 = ip[0].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U16_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U16_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U16_CONST_CONST_PTR_PTR: {
        uint16_t a0 = ip[0].u16;
        uint16_t a1 = ip[1].u16;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_U16_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_U16_CONST: {
        uint16_t a0 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_U16_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_U16_CONST: {
        uint16_t a0 = (ip++)->u16;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_U16_VOID: {
        locals[(ip++)->reg].u16 = (uint16_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_U16_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u16 = (uint16_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_U16_CONST: {
        uint16_t a0 = (ip++)->u16;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u16 = (uint16_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U16_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U16_CONST: {
        uint16_t a0 = (ip++)->u16;
        locals[(ip++)->reg].u16 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U16_REG_REG: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U16_REG_CONST: {
        uint16_t a0 = locals[(ip++)->reg].u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U16_CONST_REG: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = locals[(ip++)->reg].u16;
        locals[(ip++)->reg].u16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U16_CONST_CONST: {
        uint16_t a0 = (ip++)->u16;
        uint16_t a1 = (ip++)->u16;
        locals[(ip++)->reg].u16 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_U32_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U32_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U32_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U32_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U32_REG_PTR_PTR: {
        uint32_t a0 = locals[ip[0].reg].u32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_U32_CONST_PTR_PTR: {
        uint32_t a0 = ip[0].u32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_REG_REG_PTR_PTR: {
        uint32_t a0 = locals[ip[0].reg].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_REG_CONST_PTR_PTR: {
        uint32_t a0 = locals[ip[0].reg].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_CONST_REG_PTR_PTR: {
        uint32_t a0 = ip[0].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U32_CONST_CONST_PTR_PTR: {
        uint32_t a0 = ip[0].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U32_REG_REG_PTR_PTR: {
        uint32_t a0 = locals[ip[0].reg].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U32_REG_CONST_PTR_PTR: {
        uint32_t a0 = locals[ip[0].reg].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U32_CONST_REG_PTR_PTR: {
        uint32_t a0 = ip[0].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U32_CONST_CONST_PTR_PTR: {
        uint32_t a0 = ip[0].u32;
        uint32_t a1 = ip[1].u32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_U32_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_U32_CONST: {
        uint32_t a0 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_U32_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_U32_CONST: {
        uint32_t a0 = (ip++)->u32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_U32_VOID: {
        locals[(ip++)->reg].u32 = (uint32_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_U32_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u32 = (uint32_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_U32_CONST: {
        uint32_t a0 = (ip++)->u32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u32 = (uint32_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U32_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U32_CONST: {
        uint32_t a0 = (ip++)->u32;
        locals[(ip++)->reg].u32 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U32_REG_REG: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U32_REG_CONST: {
        uint32_t a0 = locals[(ip++)->reg].u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U32_CONST_REG: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = locals[(ip++)->reg].u32;
        locals[(ip++)->reg].u32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U32_CONST_CONST: {
        uint32_t a0 = (ip++)->u32;
        uint32_t a1 = (ip++)->u32;
        locals[(ip++)->reg].u32 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_MOD_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 % a1;
        goto redo;
    }
    case VM_OPCODE_BB_U64_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U64_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U64_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_U64_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_U64_REG_PTR_PTR: {
        uint64_t a0 = locals[ip[0].reg].u64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_U64_CONST_PTR_PTR: {
        uint64_t a0 = ip[0].u64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_REG_REG_PTR_PTR: {
        uint64_t a0 = locals[ip[0].reg].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_REG_CONST_PTR_PTR: {
        uint64_t a0 = locals[ip[0].reg].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_CONST_REG_PTR_PTR: {
        uint64_t a0 = ip[0].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_U64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_U64_CONST_CONST_PTR_PTR: {
        uint64_t a0 = ip[0].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U64_REG_REG_PTR_PTR: {
        uint64_t a0 = locals[ip[0].reg].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U64_REG_CONST_PTR_PTR: {
        uint64_t a0 = locals[ip[0].reg].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U64_CONST_REG_PTR_PTR: {
        uint64_t a0 = ip[0].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_U64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_U64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_U64_CONST_CONST_PTR_PTR: {
        uint64_t a0 = ip[0].u64;
        uint64_t a1 = ip[1].u64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_U64_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_U64_CONST: {
        uint64_t a0 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_U64_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_U64_CONST: {
        uint64_t a0 = (ip++)->u64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_U64_VOID: {
        locals[(ip++)->reg].u64 = (uint64_t) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_U64_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u64 = (uint64_t) a0;
        goto redo;
    }
    case VM_OPCODE_RET_U64_CONST: {
        uint64_t a0 = (ip++)->u64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].u64 = (uint64_t) a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U64_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BNOT_U64_CONST: {
        uint64_t a0 = (ip++)->u64;
        locals[(ip++)->reg].u64 = ~a0;
        goto redo;
    }
    case VM_OPCODE_BOR_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BOR_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 | a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BXOR_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 ^ a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BAND_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 & a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHL_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 >> a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U64_REG_REG: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U64_REG_CONST: {
        uint64_t a0 = locals[(ip++)->reg].u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U64_CONST_REG: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = locals[(ip++)->reg].u64;
        locals[(ip++)->reg].u64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_BSHR_U64_CONST_CONST: {
        uint64_t a0 = (ip++)->u64;
        uint64_t a1 = (ip++)->u64;
        locals[(ip++)->reg].u64 = a0 << a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F32_REG_REG: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F32_REG_CONST: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F32_CONST_REG: {
        float a0 = (ip++)->f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F32_CONST_CONST: {
        float a0 = (ip++)->f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F32_REG_REG: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F32_REG_CONST: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F32_CONST_REG: {
        float a0 = (ip++)->f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F32_CONST_CONST: {
        float a0 = (ip++)->f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F32_REG_REG: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F32_REG_CONST: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F32_CONST_REG: {
        float a0 = (ip++)->f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F32_CONST_CONST: {
        float a0 = (ip++)->f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F32_REG_REG: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F32_REG_CONST: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F32_CONST_REG: {
        float a0 = (ip++)->f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F32_CONST_CONST: {
        float a0 = (ip++)->f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_F32_REG_REG: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = fmodf(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F32_REG_CONST: {
        float a0 = locals[(ip++)->reg].f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = fmodf(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F32_CONST_REG: {
        float a0 = (ip++)->f32;
        float a1 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = fmodf(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F32_CONST_CONST: {
        float a0 = (ip++)->f32;
        float a1 = (ip++)->f32;
        locals[(ip++)->reg].f32 = fmodf(a0, a1);
        goto redo;
    }
    case VM_OPCODE_BB_F32_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_F32_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_F32_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_F32_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_F32_REG_PTR_PTR: {
        float a0 = locals[ip[0].reg].f32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_F32_CONST_PTR_PTR: {
        float a0 = ip[0].f32;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_REG_REG_PTR_PTR: {
        float a0 = locals[ip[0].reg].f32;
        float a1 = ip[1].f32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_REG_CONST_PTR_PTR: {
        float a0 = locals[ip[0].reg].f32;
        float a1 = ip[1].f32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_CONST_REG_PTR_PTR: {
        float a0 = ip[0].f32;
        float a1 = ip[1].f32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F32_CONST_CONST_PTR_PTR: {
        float a0 = ip[0].f32;
        float a1 = ip[1].f32;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F32_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F32_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F32_REG_REG_PTR_PTR: {
        float a0 = locals[ip[0].reg].f32;
        float a1 = ip[1].f32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F32_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F32_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F32_REG_CONST_PTR_PTR: {
        float a0 = locals[ip[0].reg].f32;
        float a1 = ip[1].f32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F32_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F32_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F32_CONST_REG_PTR_PTR: {
        float a0 = ip[0].f32;
        float a1 = ip[1].f32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F32_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F32_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F32_CONST_CONST_PTR_PTR: {
        float a0 = ip[0].f32;
        float a1 = ip[1].f32;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_F32_REG: {
        float a0 = locals[(ip++)->reg].f32;
        locals[(ip++)->reg].f32 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_F32_CONST: {
        float a0 = (ip++)->f32;
        locals[(ip++)->reg].f32 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_F32_REG: {
        float a0 = locals[(ip++)->reg].f32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_F32_CONST: {
        float a0 = (ip++)->f32;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_F32_VOID: {
        locals[(ip++)->reg].f32 = (float) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_F32_REG: {
        float a0 = locals[(ip++)->reg].f32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].f32 = (float) a0;
        goto redo;
    }
    case VM_OPCODE_RET_F32_CONST: {
        float a0 = (ip++)->f32;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].f32 = (float) a0;
        goto redo;
    }
    case VM_OPCODE_ADD_F64_REG_REG: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F64_REG_CONST: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F64_CONST_REG: {
        double a0 = (ip++)->f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_ADD_F64_CONST_CONST: {
        double a0 = (ip++)->f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 + a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F64_REG_REG: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F64_REG_CONST: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F64_CONST_REG: {
        double a0 = (ip++)->f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_SUB_F64_CONST_CONST: {
        double a0 = (ip++)->f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 - a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F64_REG_REG: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F64_REG_CONST: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F64_CONST_REG: {
        double a0 = (ip++)->f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_MUL_F64_CONST_CONST: {
        double a0 = (ip++)->f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 * a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F64_REG_REG: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F64_REG_CONST: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F64_CONST_REG: {
        double a0 = (ip++)->f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_DIV_F64_CONST_CONST: {
        double a0 = (ip++)->f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0 / a1;
        goto redo;
    }
    case VM_OPCODE_MOD_F64_REG_REG: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = fmod(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F64_REG_CONST: {
        double a0 = locals[(ip++)->reg].f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = fmod(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F64_CONST_REG: {
        double a0 = (ip++)->f64;
        double a1 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = fmod(a0, a1);
        goto redo;
    }
    case VM_OPCODE_MOD_F64_CONST_CONST: {
        double a0 = (ip++)->f64;
        double a1 = (ip++)->f64;
        locals[(ip++)->reg].f64 = fmod(a0, a1);
        goto redo;
    }
    case VM_OPCODE_BB_F64_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_F64_REG_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_F64_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BB_F64_CONST_PTR_PTR;
        ip[1].ptr = vm_run_comp(state, ip[1].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BB_F64_REG_PTR_PTR: {
        double a0 = locals[ip[0].reg].f64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BB_F64_CONST_PTR_PTR: {
        double a0 = ip[0].f64;
        if (a0 != 0) {
            ip = ip[1].ptr;
        } else {
            ip = ip[2].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_REG_REG_PTR_PTR: {
        double a0 = locals[ip[0].reg].f64;
        double a1 = ip[1].f64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_REG_CONST_PTR_PTR: {
        double a0 = locals[ip[0].reg].f64;
        double a1 = ip[1].f64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_CONST_REG_PTR_PTR: {
        double a0 = ip[0].f64;
        double a1 = ip[1].f64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BEQ_F64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BEQ_F64_CONST_CONST_PTR_PTR: {
        double a0 = ip[0].f64;
        double a1 = ip[1].f64;
        if (a0 == a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F64_REG_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F64_REG_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F64_REG_REG_PTR_PTR: {
        double a0 = locals[ip[0].reg].f64;
        double a1 = ip[1].f64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F64_REG_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F64_REG_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F64_REG_CONST_PTR_PTR: {
        double a0 = locals[ip[0].reg].f64;
        double a1 = ip[1].f64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F64_CONST_REG_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F64_CONST_REG_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F64_CONST_REG_PTR_PTR: {
        double a0 = ip[0].f64;
        double a1 = ip[1].f64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_BLT_F64_CONST_CONST_FUNC_FUNC: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_BLT_F64_CONST_CONST_PTR_PTR;
        ip[2].ptr = vm_run_comp(state, ip[2].func);
        ip[3].ptr = vm_run_comp(state, ip[3].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_BLT_F64_CONST_CONST_PTR_PTR: {
        double a0 = ip[0].f64;
        double a1 = ip[1].f64;
        if (a0 < a1) {
            ip = ip[2].ptr;
        } else {
            ip = ip[3].ptr;
        }
        goto redo;
    }
    case VM_OPCODE_MOVE_F64_REG: {
        double a0 = locals[(ip++)->reg].f64;
        locals[(ip++)->reg].f64 = a0;
        goto redo;
    }
    case VM_OPCODE_MOVE_F64_CONST: {
        double a0 = (ip++)->f64;
        locals[(ip++)->reg].f64 = a0;
        goto redo;
    }
    case VM_OPCODE_OUT_F64_REG: {
        double a0 = locals[(ip++)->reg].f64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_OUT_F64_CONST: {
        double a0 = (ip++)->f64;
        putchar((int) a0);
        goto redo;
    }
    case VM_OPCODE_IN_F64_VOID: {
        locals[(ip++)->reg].f64 = (double) fgetc(stdin);
        goto redo;
    }
    case VM_OPCODE_RET_F64_REG: {
        double a0 = locals[(ip++)->reg].f64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].f64 = (double) a0;
        goto redo;
    }
    case VM_OPCODE_RET_F64_CONST: {
        double a0 = (ip++)->f64;
        locals -= VM_NREGS;
        ip = *(ips--);
        locals[(ip++)->reg].f64 = (double) a0;
        goto redo;
    }
    case VM_OPCODE_EXIT_BREAK_VOID: {
        {return;}
        goto redo;
    }
    case VM_OPCODE_JUMP_PTR_CONST: {
            ip = ip[0].ptr;
        goto redo;
    }
    case VM_OPCODE_JUMP_FUNC_CONST: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_JUMP_PTR_CONST;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals[263] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals[263] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    case VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *t0 = (ip++)->ptr;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals[263] = locals[(ip++)->reg];
        locals[264] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = t0;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_CONST_REG_REG_REG_REG_REG_REG_REG_REG: {
        vm_opcode_t *head = ip-1;
        head->reg = VM_OPCODE_CALL_PTR_CONST_REG_REG_REG_REG_REG_REG_REG_REG;
        ip[0].ptr = vm_run_comp(state, ip[0].func);
        ip = head;
        goto redo;
    }
    case VM_OPCODE_CALL_FUNC_REG_REG_REG_REG_REG_REG_REG_REG_REG: {
        vm_rblock_t *t0 = (ip++)->func;
        locals[257] = locals[(ip++)->reg];
        locals[258] = locals[(ip++)->reg];
        locals[259] = locals[(ip++)->reg];
        locals[260] = locals[(ip++)->reg];
        locals[261] = locals[(ip++)->reg];
        locals[262] = locals[(ip++)->reg];
        locals[263] = locals[(ip++)->reg];
        locals[264] = locals[(ip++)->reg];
        locals += VM_NREGS;
        *(++ips) = ip;
        ip = vm_run_comp(state, t0);
        goto redo;
    }
    }
}