#include "x64.h"

const uint8_t vm_x64_instr_argc_table[VM_X64_OPCODE_MAX_OPCODE] = {
    [VM_X64_OPCODE_CMP] = 2,
    [VM_X64_OPCODE_TEST] = 2,
    [VM_X64_OPCODE_JMP] = 1,
    [VM_X64_OPCODE_JE] = 1,
    [VM_X64_OPCODE_JL] = 1,
    [VM_X64_OPCODE_JB] = 1,
    [VM_X64_OPCODE_ADD] = 2,
    [VM_X64_OPCODE_SUB] = 2,
    [VM_X64_OPCODE_MUL] = 1,
    [VM_X64_OPCODE_DIV] = 1,
    [VM_X64_OPCODE_MOV] = 2,
    [VM_X64_OPCODE_PUSH] = 1,
    [VM_X64_OPCODE_POP] = 1,
    [VM_X64_OPCODE_CALL] = 1,
    [VM_X64_OPCODE_XOR] = 2,
};

void vm_x64_print_opcode(FILE *file,vm_x64_opcode_t opcode) {
    switch (opcode) {
    case VM_X64_OPCODE_CMP: {
        fprintf(file, "cmp");
        break;
    }
    case VM_X64_OPCODE_TEST: {
        fprintf(file, "test");
        break;
    }
    case VM_X64_OPCODE_JMP: {
        fprintf(file, "jmp");
        break;
    }
    case VM_X64_OPCODE_JE: {
        fprintf(file, "je");
        break;
    }
    case VM_X64_OPCODE_JL: {
        fprintf(file, "jl");
        break;
    }
    case VM_X64_OPCODE_JB: {
        fprintf(file, "jb");
        break;
    }
    case VM_X64_OPCODE_ADD: {
        fprintf(file, "add");
        break;
    }
    case VM_X64_OPCODE_SUB: {
        fprintf(file, "sub");
        break;
    }
    case VM_X64_OPCODE_MUL: {
        fprintf(file, "mul");
        break;
    }
    case VM_X64_OPCODE_DIV: {
        fprintf(file, "div");
        break;
    }
    case VM_X64_OPCODE_MOV: {
        fprintf(file, "mov");
        break;
    }
    case VM_X64_OPCODE_PUSH: {
        fprintf(file, "push");
        break;
    }
    case VM_X64_OPCODE_POP: {
        fprintf(file, "pop");
        break;
    }
    case VM_X64_OPCODE_CALL: {
        fprintf(file, "call");
        break;
    }
    case VM_X64_OPCODE_XOR: {
        fprintf(file, "xor");
        break;
    }
    }
}

void vm_x64_print_reg(FILE *file, uint8_t num, uint8_t bitsize) {
    static const char *table[8] = {
        "ax",
        "cx",
        "dx",
        "bx",
        "sp",
        "bp",
        "si",
        "di",
    };
    if (num < 8) {
        if (bitsize == 8) {
            fprintf(file, "%cl", *table[num]);
        } else if (bitsize == 16) {
            fprintf(file, "%s", table[num]);
        } else if (bitsize == 32) {
            fprintf(file, "e%s", table[num]);
        } else if (bitsize == 64) {
            fprintf(file, "r%s", table[num]);
        } else {
            __builtin_trap();
        }
    } else {
        if (bitsize == 8) {
            fprintf(file, "r%ib", num);
        } else if (bitsize == 16) {
            fprintf(file, "r%iw", num);
        } else if (bitsize == 32) {
            fprintf(file, "r%id", num);
        } else if (bitsize == 64) {
            fprintf(file, "r%i", num);
        } else {
            __builtin_trap();
        }
    }
}

void vm_x64_print_instr(FILE *file, vm_x64_instr_t instr) {
    vm_x64_print_opcode(file, instr.opcode);
    for (uint8_t i = 0; i < vm_x64_instr_argc_table[instr.opcode]; i++) {
        fprintf(file, " ");
        vm_x64_arg_t arg = instr.args[i];
        switch (arg.type) {
        case VM_X64_ARG_REG: {
            vm_x64_print_reg(file, arg.reg, instr.bitsize);
            break;
        }
        case VM_X64_ARG_LOAD: {
            if (instr.bitsize == 8) {
                fprintf(file, "byte");
            }
            if (instr.bitsize == 16) {
                fprintf(file, "word");
            }
            if (instr.bitsize == 32) {
                fprintf(file, "dword");
            }
            if (instr.bitsize == 64) {
                fprintf(file, "qword");
            }
            fprintf(file, " [");
            vm_x64_print_reg(file, arg.reg, 64);
            fprintf(file, "]");
            break;
        }
        case VM_X64_ARG_LABEL: {
            fprintf(file, "=>%"PRIu32, arg.label);
            break;
        }
        case VM_X64_ARG_IMM32: {
            fprintf(file, "%"PRIu32, arg.imm32);
            break;
        }
        case VM_X64_ARG_IMM64: {
            fprintf(file, "%"PRIu64, arg.imm64);
            break;
        }
        }
    }
    fprintf(file, "\n");
}

void vm_x64_print_instr_buf(FILE *file, vm_x64_instr_buf_t buf) {
    for (size_t i = 0; i < buf.len; i++) {
        vm_x64_print_instr(file, buf.instrs[i]);
    }
}
