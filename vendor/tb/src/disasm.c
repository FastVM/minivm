#include <tb_x64.h>

ptrdiff_t tb_print_disassembly_inst(TB_Arch arch, size_t length, const void* ptr) {
    switch (arch) {
        #ifdef TB_HAS_X64
        case TB_ARCH_X86_64: {
            TB_X86_Inst inst;
            if (!tb_x86_disasm(&inst, length, ptr)) {
                return -1;
            }

            const char* mnemonic = tb_x86_mnemonic(&inst);
            printf("%s", mnemonic);
            if (inst.data_type >= TB_X86_TYPE_SSE_SS && inst.data_type <= TB_X86_TYPE_SSE_PD) {
                static const char* strs[] = { "ss", "sd", "ps", "pd" };
                printf("%s", strs[inst.data_type - TB_X86_TYPE_SSE_SS]);
            }
            printf(" ");

            bool mem = true, imm = true;
            for (int i = 0; i < 4; i++) {
                if (inst.regs[i] == -1) {
                    if (mem && (inst.flags & TB_X86_INSTR_USE_MEMOP)) {
                        if (i > 0) printf(", ");

                        mem = false;
                        if (inst.flags & TB_X86_INSTR_USE_RIPMEM) {
                            if (inst.disp < 0) {
                                printf("rip - %d", -inst.disp);
                            } else {
                                printf("rip + %d", inst.disp);
                            }
                        } else {
                            printf("%s [", tb_x86_type_name(inst.data_type));
                            if (inst.base != 255) {
                                printf("%s", tb_x86_reg_name(inst.base, TB_X86_TYPE_QWORD));
                            }

                            if (inst.index != 255) {
                                printf(" + %s*%d", tb_x86_reg_name(inst.index, TB_X86_TYPE_QWORD), 1 << inst.scale);
                            }

                            if (inst.disp > 0) {
                                printf(" + %d", inst.disp);
                            } else if (inst.disp < 0) {
                                printf(" - %d", -inst.disp);
                            }

                            printf("]");
                        }
                    } else if (imm && (inst.flags & (TB_X86_INSTR_IMMEDIATE | TB_X86_INSTR_ABSOLUTE))) {
                        if (i > 0) printf(", ");

                        imm = false;
                        printf("%d", inst.imm);
                    } else {
                        break;
                    }
                } else {
                    if (i > 0) {
                        printf(", ");

                        // special case for certain ops with two data types
                        if (inst.flags & TB_X86_INSTR_TWO_DATA_TYPES) {
                            printf("%s", tb_x86_reg_name(inst.regs[i], inst.data_type2));
                            continue;
                        }
                    }

                    printf("%s", tb_x86_reg_name(inst.regs[i], inst.data_type));
                }
            }

            printf("\n");
            return inst.length;
        }
        #endif

        default:
        tb_todo();
    }
}

