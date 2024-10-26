
#include "io.h"
#include "ir.h"
#include "lib.h"

void vm_block_realloc(vm_ir_block_t *block, vm_ir_instr_t instr) {
    if (block->len + 4 >= block->alloc) {
        block->alloc = (block->len + 4) * 4;
        block->instrs =
            vm_realloc(block->instrs, sizeof(vm_ir_instr_t) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

void vm_io_format_arg(vm_io_buffer_t *out, vm_ir_arg_t val) {
    switch (val.type) {
        case VM_IR_ARG_TYPE_LIT: {
            vm_io_buffer_print_lit(out, val.lit);
            break;
        }
        case VM_IR_ARG_TYPE_REG: {
            vm_io_buffer_format(out, "%%%zu", (size_t)val.reg);
            break;
        }
    }
}

void vm_io_format_branch(vm_io_buffer_t *out, vm_ir_branch_t val) {
    if (val.out.type != VM_IR_ARG_TYPE_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_IR_BRANCH_OPCODE_JUMP: {
            vm_io_buffer_format(out, "jump");
            break;
        }
        case VM_IR_BRANCH_OPCODE_BOOL: {
            vm_io_buffer_format(out, "bb");
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_LT: {
            vm_io_buffer_format(out, "blt");
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_LE: {
            vm_io_buffer_format(out, "ble");
            break;
        }
        case VM_IR_BRANCH_OPCODE_IF_EQ: {
            vm_io_buffer_format(out, "beq");
            break;
        }
        case VM_IR_BRANCH_OPCODE_RETURN: {
            vm_io_buffer_format(out, "ret");
            break;
        }
        case VM_IR_BRANCH_OPCODE_TABLE_GET: {
            vm_io_buffer_format(out, "get");
            break;
        }
        case VM_IR_BRANCH_OPCODE_CAPTURE_LOAD: {
            vm_io_buffer_format(out, "load");
            break;
        }
        case VM_IR_BRANCH_OPCODE_CALL: {
            vm_io_buffer_format(out, "call");
            break;
        }
    }
    if (val.op == VM_IR_BRANCH_OPCODE_CALL) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[0]);
        vm_io_buffer_format(out, "(");
        for (size_t i = 1; val.args[i].type != VM_IR_ARG_TYPE_NONE; i++) {
            if (i != 1) {
                vm_io_buffer_format(out, ", ");
            }
            vm_io_format_arg(out, val.args[i]);
        }
        vm_io_buffer_format(out, ")");
    } else {
        for (size_t i = 0; val.args[i].type != VM_IR_ARG_TYPE_NONE; i++) {
            vm_io_buffer_format(out, " ");
            vm_io_format_arg(out, val.args[i]);
        }
    }
    if (val.op == VM_IR_BRANCH_OPCODE_CAPTURE_LOAD || val.op == VM_IR_BRANCH_OPCODE_TABLE_GET || val.op == VM_IR_BRANCH_OPCODE_CALL) {
        vm_io_buffer_format(out, " then .%zi", (size_t)val.targets[0]->id);
    } else {
        if (val.targets[0]) {
            vm_io_buffer_format(out, " .%zi", (size_t)val.targets[0]->id);
        }
        if (val.targets[1]) {
            vm_io_buffer_format(out, " .%zi", (size_t)val.targets[1]->id);
        }
    }
}

void vm_io_format_instr(vm_io_buffer_t *out, vm_ir_instr_t val) {
    if (val.op == VM_IR_INSTR_OPCODE_NOP) {
        vm_io_buffer_format(out, "nop");
        return;
    }
    if (val.out.type != VM_IR_ARG_TYPE_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_IR_INSTR_OPCODE_MOVE: {
            vm_io_buffer_format(out, "move");
            break;
        }
        case VM_IR_INSTR_OPCODE_ADD: {
            vm_io_buffer_format(out, "add");
            break;
        }
        case VM_IR_INSTR_OPCODE_SUB: {
            vm_io_buffer_format(out, "sub");
            break;
        }
        case VM_IR_INSTR_OPCODE_MUL: {
            vm_io_buffer_format(out, "mul");
            break;
        }
        case VM_IR_INSTR_OPCODE_DIV: {
            vm_io_buffer_format(out, "div");
            break;
        }
        case VM_IR_INSTR_OPCODE_IDIV: {
            vm_io_buffer_format(out, "idiv");
            break;
        }
        case VM_IR_INSTR_OPCODE_MOD: {
            vm_io_buffer_format(out, "mod");
            break;
        }
        case VM_IR_INSTR_OPCODE_TABLE_SET: {
            vm_io_buffer_format(out, "set");
            break;
        }
        case VM_IR_INSTR_OPCODE_TABLE_NEW: {
            vm_io_buffer_format(out, "new");
            break;
        }
        case VM_IR_INSTR_OPCODE_TABLE_LEN: {
            vm_io_buffer_format(out, "len");
            break;
        }
        default: {
            vm_io_buffer_format(out, "<instr: %zu>", (size_t)val.op);
            break;
        }
    }
    for (size_t i = 0; val.args[i].type != VM_IR_ARG_TYPE_NONE; i++) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[i]);
    }
}

void vm_io_format_block(vm_io_buffer_t *out, vm_ir_block_t *val) {
    if (val == NULL) {
        printf("<block: null>\n");
    }
    vm_io_buffer_format(out, ".%zi:\n", (ptrdiff_t)val->id);
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i].op == VM_IR_INSTR_OPCODE_NOP) {
            continue;
        }
        vm_io_buffer_format(out, "    ");
        vm_io_format_instr(out, val->instrs[i]);
        vm_io_buffer_format(out, "\n");
    }
    if (val->branch.op != VM_IR_BRANCH_OPCODE_FALL) {
        vm_io_buffer_format(out, "    ");
        vm_io_format_branch(out, val->branch);
        vm_io_buffer_format(out, "\n");
    } else {
        vm_io_buffer_format(out, "    <fall>\n");
    }
}

enum {
    VM_INFO_REG_UNK,
    VM_INFO_REG_DEF,
    VM_INFO_REG_ARG,
};
