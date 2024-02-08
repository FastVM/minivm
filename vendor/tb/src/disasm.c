#include <tb_x64.h>

ptrdiff_t tb_print_disassembly_inst(TB_Arch arch, size_t length, const void* ptr) {
    switch (arch) {
        #ifdef TB_HAS_X64
        case TB_ARCH_X86_64: {
            TB_X86_Inst inst;
            if (!tb_x86_disasm(&inst, length, ptr)) {
                return -1;
            }

            tb_x86_print_inst(stdout, &inst);
            return inst.length;
        }
        #endif

        default:
        tb_todo();
    }
}

