
#include "./ir.h"
#include "./io.h"

void vm_block_realloc(vm_block_t *block, vm_instr_t instr) {
    if (block->len + 4 >= block->alloc) {
        block->alloc = (block->len + 4) * 4;
        block->instrs =
            vm_realloc(block->instrs, sizeof(vm_instr_t) * block->alloc);
    }
    block->instrs[block->len++] = instr;
}

void vm_io_format_arg(vm_io_buffer_t *out, vm_arg_t val) {
    switch (val.type) {
        case VM_ARG_LIT: {
            vm_io_print_lit(out, val.lit);
            break;
        }
        case VM_ARG_REG: {
            vm_io_buffer_format(out, "%%%zu", (size_t)val.reg);
            break;
        }
    }
}

void vm_io_format_branch(vm_io_buffer_t *out, vm_branch_t val) {
    if (val.out.type != VM_ARG_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_BOP_JUMP: {
            vm_io_buffer_format(out, "jump");
            break;
        }
        case VM_BOP_BB: {
            vm_io_buffer_format(out, "bb");
            break;
        }
        case VM_BOP_BLT: {
            vm_io_buffer_format(out, "blt");
            break;
        }
        case VM_BOP_BLE: {
            vm_io_buffer_format(out, "ble");
            break;
        }
        case VM_BOP_BEQ: {
            vm_io_buffer_format(out, "beq");
            break;
        }
        case VM_BOP_RET: {
            vm_io_buffer_format(out, "ret");
            break;
        }
        case VM_BOP_GET: {
            vm_io_buffer_format(out, "get");
            break;
        }
        case VM_BOP_LOAD: {
            vm_io_buffer_format(out, "load");
            break;
        }
        case VM_BOP_CALL: {
            vm_io_buffer_format(out, "call");
            break;
        }
    }
    if (val.op == VM_BOP_CALL) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[0]);
        vm_io_buffer_format(out, "(");
        for (size_t i = 1; val.args[i].type != VM_ARG_NONE; i++) {
            if (i != 1) {
                vm_io_buffer_format(out, ", ");
            }
            vm_io_format_arg(out, val.args[i]);
        }
        vm_io_buffer_format(out, ")");
    } else {
        for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
            vm_io_buffer_format(out, " ");
            vm_io_format_arg(out, val.args[i]);
        }
    }
    if (val.op == VM_BOP_LOAD || val.op == VM_BOP_GET || val.op == VM_BOP_CALL) {
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

void vm_io_format_instr(vm_io_buffer_t *out, vm_instr_t val) {
    if (val.op == VM_IOP_NOP) {
        vm_io_buffer_format(out, "nop");
        return;
    }
    if (val.out.type != VM_ARG_NONE) {
        vm_io_format_arg(out, val.out);
        vm_io_buffer_format(out, " <- ");
    }
    switch (val.op) {
        case VM_IOP_MOVE: {
            vm_io_buffer_format(out, "move");
            break;
        }
        case VM_IOP_ADD: {
            vm_io_buffer_format(out, "add");
            break;
        }
        case VM_IOP_SUB: {
            vm_io_buffer_format(out, "sub");
            break;
        }
        case VM_IOP_MUL: {
            vm_io_buffer_format(out, "mul");
            break;
        }
        case VM_IOP_DIV: {
            vm_io_buffer_format(out, "div");
            break;
        }
        case VM_IOP_IDIV: {
            vm_io_buffer_format(out, "idiv");
            break;
        }
        case VM_IOP_MOD: {
            vm_io_buffer_format(out, "mod");
            break;
        }
        case VM_IOP_TABLE_SET: {
            vm_io_buffer_format(out, "set");
            break;
        }
        case VM_IOP_TABLE_NEW: {
            vm_io_buffer_format(out, "new");
            break;
        }
        case VM_IOP_STD: {
            vm_io_buffer_format(out, "std");
            break;
        }
        case VM_IOP_TABLE_LEN: {
            vm_io_buffer_format(out, "len");
            break;
        }
        default: {
            vm_io_buffer_format(out, "<instr: %zu>", (size_t)val.op);
            break;
        }
    }
    for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
        vm_io_buffer_format(out, " ");
        vm_io_format_arg(out, val.args[i]);
    }
}

void vm_io_format_block(vm_io_buffer_t *out, vm_block_t *val) {
    if (val == NULL) {
        printf("<block: null>\n");
    }
    vm_io_buffer_format(out, ".%zi:\n", (ptrdiff_t)val->id);
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i].op == VM_IOP_NOP) {
            continue;
        }
        vm_io_buffer_format(out, "    ");
        vm_io_format_instr(out, val->instrs[i]);
        vm_io_buffer_format(out, "\n");
    }
    if (val->branch.op != VM_BOP_FALL) {
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
