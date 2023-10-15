
#include "./os.h"
#include "./io.h"
#include "../util.h"
#include "./dot.h"
#include "../../ir.h"
#include "../../jit/tb.h"
#include "../../lang/paka.h"

typedef struct {
    size_t nblocks;
    vm_block_t **blocks;
    size_t blocks_alloc;
    size_t func_depth;
    size_t nfuncs;
    ptrdiff_t *funcs;
    size_t funcs_alloc;
} vm_dot_list_t;

static void vm_dot_draw_tag(FILE *out, vm_dot_list_t *list, vm_tag_t tag) {
    (void) list;
    switch (tag) {
        case VM_TAG_UNK: {
            fprintf(out, "unk");
            break;
        }
        case VM_TAG_NIL: {
            fprintf(out, "nil");
            break;
        }
        case VM_TAG_BOOL: {
            fprintf(out, "bool");
            break;
        }
        case VM_TAG_I64: {
            fprintf(out, "i64");
            break;
        }
        case VM_TAG_F64: {
            fprintf(out, "f64");
            break;
        }
        case VM_TAG_STR: {
            fprintf(out, "str");
            break;
        }
        case VM_TAG_FUN: {
            fprintf(out, "fun");
            break;
        }
        case VM_TAG_TAB: {
            fprintf(out, "tab");
            break;
        }
        case VM_TAG_FFI: {
            fprintf(out, "ffi");
            break;
        }
    }
}


static void vm_dot_draw_arg(FILE *out, vm_dot_list_t *list, vm_arg_t val) {
    switch (val.type) {
        case VM_ARG_NIL: {
            fprintf(out, "nil");
            break;
        }
        case VM_ARG_BOOL: {
            fprintf(out, "%s", val.logic ? "true" : "false");
            break;
        }
        case VM_ARG_NUM: {
            fprintf(out, "%lf", val.num);
            break;
        }
        case VM_ARG_STR: {
            fprintf(out, "\\\"");
            for (const char *buf = val.str; *buf != '\0'; buf++) {
                if (*buf == '"') {
                    fprintf(out, "\\\"");
                } else {
                    fprintf(out, "%c", *buf);
                }
            }
            fprintf(out, "\\\"");
            break;
        }
        case VM_ARG_REG: {
            if (val.reg_tag == VM_TAG_UNK) {
                fprintf(out, "r%zu", (size_t)val.reg);
            } else {
                fprintf(out, "r%zu:", (size_t)val.reg);
                vm_dot_draw_tag(out, list, val.reg_tag);
            }
            break;
        }
        case VM_ARG_FFI: {
            fprintf(out, "<ffi.func>");
            break;
        }
        case VM_ARG_FUNC: {
            fprintf(out, "block%zi", val.func->id);
            break;
        }
        case VM_ARG_RFUNC: {
            fprintf(out, "block%zi", val.rfunc->block->id);
            break;
        }
        case VM_ARG_CPU0: {
            fprintf(out, "(rax|xmm0)r%zu", (size_t)val.reg);
            break;
        }
        case VM_ARG_CPU_GP: {
            const char *names[16] = {
                "rax",
                "rcx",
                "rdx",
                "rbx",
                "rsp",
                "rbp",
                "rsi",
                "rdi",
                "r8",
                "r9",
                "r10",
                "r11",
                "r12",
                "r13",
                "r14",
                "r15",
            };
            fprintf(out, "%sr%zu", names[val.r64], (size_t)val.reg);
            break;
        }
        case VM_ARG_CPU_FP: {
            fprintf(out, "xmm%zur%zu", (size_t)val.f64, (size_t)val.reg);
            break;
        }
    }
}

static void vm_dot_draw_branch(FILE *out, vm_dot_list_t *list, vm_branch_t val) {
    if (val.out.type != VM_ARG_NONE) {
        vm_dot_draw_arg(out, list, val.out);
        fprintf(out, " = ");
    }
    size_t n = 0;
    switch (val.op) {
        case VM_BOP_JUMP: {
            fprintf(out, "jump");
            n = 1;
            break;
        }
        case VM_BOP_BB: {
            fprintf(out, "bb");
            n = 2;
            break;
        }
        case VM_BOP_BTYPE: {
            fprintf(out, "btype");
            n = 2;
            break;
        }
        case VM_BOP_BLT: {
            fprintf(out, "blt");
            n = 2;
            break;
        }
        case VM_BOP_BEQ: {
            fprintf(out, "beq");
            n = 2;
            break;
        }
        case VM_BOP_RET: {
            fprintf(out, "ret");
            n = 0;
            break;
        }
        case VM_BOP_GET: {
            fprintf(out, "get");
            n = 1;
            break;
        }
        case VM_BOP_CALL: {
            fprintf(out, "call");
            n = 1;
            break;
        }
    }
    if (val.tag != VM_TAG_UNK) {
        fprintf(out, ".");
        vm_dot_draw_tag(out, list, val.tag);
    }
    if (val.op == VM_BOP_CALL) {
        fprintf(out, " ");
        vm_dot_draw_arg(out, list, val.args[0]);
        fprintf(out, "(");
        for (size_t i = 1; val.args[i].type != VM_ARG_NONE; i++) {
            if (i != 1) {
                fprintf(out, ", ");
            }
            vm_dot_draw_arg(out, list, val.args[i]);
        }
        fprintf(out, ")");
    } else {
        for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
            fprintf(out, " ");
            vm_dot_draw_arg(out, list, val.args[i]);
        }
    }
    if (val.op == VM_BOP_GET || val.op == VM_BOP_CALL) {
        fprintf(out, " then block%zi", (size_t)val.targets[0]->id);
        fprintf(out, "(");
        for (size_t i = 0; i < val.targets[0]->nargs; i++) {
            if (i != 0) {
                fprintf(out, ", ");
            }
            vm_dot_draw_arg(out, list, val.targets[0]->args[i]);
        }
        fprintf(out, ")");
    } else {
        if (val.targets[0] && n >= 1) {
            fprintf(out, " block%zi", (size_t)val.targets[0]->id);
            fprintf(out, "(");
            for (size_t i = 0; i < val.targets[0]->nargs; i++) {
                if (i != 0) {
                    fprintf(out, ", ");
                }
                vm_dot_draw_arg(out, list, val.targets[0]->args[i]);
            }
            fprintf(out, ")");
        }
        if (val.targets[1] && n >= 2) {
            fprintf(out, " block%zi", (size_t)val.targets[1]->id);
            fprintf(out, "(");
            for (size_t i = 0; i < val.targets[1]->nargs; i++) {
                if (i != 0) {
                    fprintf(out, ", ");
                }
                vm_dot_draw_arg(out, list, val.targets[1]->args[i]);
            }
            fprintf(out, ")");
        }
    }
}

static void vm_dot_draw_instr(FILE *out, vm_dot_list_t *list, vm_instr_t val) {
    if (val.op == VM_IOP_NOP) {
        fprintf(out, "nop");
        return;
    }
    if (val.out.type != VM_ARG_NONE) {
        vm_dot_draw_arg(out, list, val.out);
        fprintf(out, " = ");
    }
    switch (val.op) {
        case VM_IOP_MOVE: {
            fprintf(out, "move");
            break;
        }
        case VM_IOP_ADD: {
            fprintf(out, "add");
            break;
        }
        case VM_IOP_SUB: {
            fprintf(out, "sub");
            break;
        }
        case VM_IOP_MUL: {
            fprintf(out, "mul");
            break;
        }
        case VM_IOP_DIV: {
            fprintf(out, "div");
            break;
        }
        case VM_IOP_MOD: {
            fprintf(out, "mod");
            break;
        }
        case VM_IOP_SET: {
            fprintf(out, "set");
            break;
        }
        case VM_IOP_NEW: {
            fprintf(out, "new");
            break;
        }
        case VM_IOP_STD: {
            fprintf(out, "std");
            break;
        }
        case VM_IOP_TYPE: {
            fprintf(out, "type");
            break;
        }
        case VM_IOP_LEN: {
            fprintf(out, "len");
            break;
        }
    }
    if (val.tag != VM_TAG_UNK) {
        fprintf(out, ".");
        vm_dot_draw_tag(out, list, val.tag);
    }
    for (size_t i = 0; val.args[i].type != VM_ARG_NONE; i++) {
        fprintf(out, " ");
        vm_dot_draw_arg(out, list, val.args[i]);
    }
}

static void vm_dot_draw_block_body(FILE *out, vm_dot_list_t *list, vm_block_t *val) {
    for (size_t i = 0; i < val->len; i++) {
        if (val->instrs[i].op == VM_IOP_NOP) {
            continue;
        }
        vm_dot_draw_instr(out, list, val->instrs[i]);
        fprintf(out, "\\l");
    }
    if (val->branch.op != VM_BOP_FALL) {
        vm_dot_draw_branch(out, list, val->branch);
        fprintf(out, "\\l");
    }
}

static void vm_dot_draw_block(FILE *out, vm_dot_list_t *list, vm_block_t *block) {
    for (size_t i = 0; i < list->nblocks; i++) {
        if (list->blocks[i] == block) {
            return;
        }
    }
    if (list->nblocks + 2 >= list->blocks_alloc) {
        list->blocks_alloc = (list->nblocks + 2) * 2;
        list->blocks = vm_realloc(list->blocks, sizeof(vm_block_t *) * list->blocks_alloc);
    }
    const char *fillcolor = list->func_depth % 2 == 0 ? "F4FFFF" : "F4FFF4";
    list->blocks[list->nblocks] = block;
    fprintf(out, "block%zu [shape=record] [label=\"{block%zu", block->id, block->id);
    fprintf(out, "(");
    for (size_t i = 0; i < block->nargs; i++) {
        if (i != 0) {
            fprintf(out, ", ");
        }
        vm_dot_draw_arg(out, list, block->args[i]);
    }
    fprintf(out, ")");
    fprintf(out, "\\n|");
    vm_dot_draw_block_body(out, list, block);
    fprintf(out, "}\"] [fillcolor=\"#%s\"] [style=\"filled\"];\n  ", fillcolor);
    list->nblocks++;
    if (block->branch.op == VM_BOP_CALL) {
        if (block->branch.args[0].type == VM_ARG_FUNC) {
            vm_block_t *func = block->branch.args[0].func;
            size_t funcnum = list->nfuncs;
            for (size_t i = 0; i < list->nfuncs; i++) {
                if (list->funcs[i] == func->id) {
                    goto call_found_func_already;
                }
            }
            fprintf(out, "subgraph clusterblock%zu {\n  ", funcnum);
            if (list->func_depth % 2 == 0) {
                fprintf(out, "bgcolor=\"#FFFFF4\"");
            } else {
                fprintf(out, "bgcolor=\"#FFFFFF\"");
            }
            fprintf(out, "label=\"func%zu\"\n  ", funcnum);
            if (list->nfuncs + 2 >= list->funcs_alloc) {
                list->funcs_alloc = (list->nfuncs + 2) * 2;
                list->funcs = vm_realloc(list->funcs, sizeof(ptrdiff_t) * list->funcs_alloc);
            }
            list->funcs[list->nfuncs++] = func->id;
            list->func_depth += 1;
            vm_dot_draw_block(out, list, func);
            list->func_depth -= 1;
            fprintf(out, "}\n  ");
        call_found_func_already:;
            fprintf(out, "block%zu -> block%zu;\n  ", block->id, func->id);
        }
    }
    if (block->branch.op == VM_BOP_RET) {
        fprintf(out, "RETURN%zu [label=\"RETURN\"] [fillcolor=\"#%s\"] [style=\"filled\"];\n  ", block->id, fillcolor);
        fprintf(out, "block%zu -> RETURN%zu;\n  ", block->id, block->id);
    } else {
        for (size_t i = 0; i < 2; i++) {
            if (block->branch.targets[i] != NULL) {
                vm_dot_draw_block(out, list, block->branch.targets[i]);
                fprintf(out, "block%zu -> block%zu;\n  ", block->id, block->branch.targets[i]->id);
            }
        }
    }
}

void vm_dot_block(FILE *file, vm_block_t *block) {
    vm_dot_list_t dot_list = (vm_dot_list_t) {
        .nblocks = 0,
        .blocks = NULL,
        .blocks_alloc = 0,
        .nfuncs = 0,
        .funcs = NULL,
        .funcs_alloc = 0,
    };
    fprintf(file, "digraph IR {\n  ");
    fprintf(file, "bgcolor=\"#FFFFFF\"");
    fprintf(file, "rankdir=\"TB\"\n  ");
    fprintf(file, "START -> block%zu;\n  ", block->id);
    vm_dot_draw_block(file, &dot_list, block);
    fprintf(file, "START [fillcolor=\"#F4FFFF\"] [style=\"filled\"];\n  EXIT [fillcolor=\"#F4FFFF\"] [style=\"filled\"];\n}");
}

vm_std_value_t vm_std_dot_parse(vm_std_value_t *args) {
    const char *src = NULL;
    const char *file = NULL;
    if (vm_std_parse_args(args, "ss", &file, &src)) {
        vm_block_t *block = vm_paka_parse(src);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            goto fail;
        }
        FILE *out = fopen(file, "w");
        vm_dot_block(out, block);
        fclose(out);
        return (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
    }
fail:;
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}

vm_std_value_t vm_std_dot_file(vm_std_value_t *args) {
    const char *name = NULL;
    const char *file = NULL;
    if (vm_std_parse_args(args, "ss", &file, &name)) {
        char *src = vm_io_read(name);
        vm_block_t *block = vm_paka_parse(src);
        if (block == NULL) {
            fprintf(stderr, "error: could not parse file\n");
            goto fail;
        }
        FILE *out = fopen(file, "w");
        vm_dot_block(out, block);
        fclose(out);
        return (vm_std_value_t){
            .tag = VM_TAG_NIL,
        };
    }
fail:;
    return (vm_std_value_t){
        .tag = VM_TAG_UNK,
    };
}
