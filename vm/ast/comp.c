

#include "comp.h"
#include "../type.h"
#include "ast.h"
#include "print.h"

struct vm_ast_comp_t;
typedef struct vm_ast_comp_t vm_ast_comp_t;

struct vm_ast_comp_t {
    vm_ast_blocks_t blocks;
    vm_block_t *cur;
    size_t nregs;
    const char **names;
    size_t names_alloc;
};

static vm_arg_t *vm_ast_args(size_t nargs, ...) {
    va_list ap;
    va_start(ap, nargs);
    vm_arg_t *ret = vm_malloc(sizeof(vm_arg_t) * (nargs + 1));
    for (size_t i = 0; i < nargs; i++) {
        ret[i] = va_arg(ap, vm_arg_t);
    }
    va_end(ap);
    ret[nargs] = (vm_arg_t){
        .type = VM_ARG_NONE,
    };
    return ret;
}

static vm_block_t *vm_ast_comp_new_block(vm_ast_comp_t *comp) {
    if (comp->blocks.len + 1 >= comp->blocks.alloc) {
        comp->blocks.alloc = (comp->blocks.len + 1) * 2;
        comp->blocks.blocks = vm_realloc(comp->blocks.blocks, sizeof(vm_block_t *) * comp->blocks.alloc);
    }
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t){
        .id = (ptrdiff_t)comp->blocks.len,
        .cache = vm_malloc(sizeof(vm_cache_t)),
    };
    vm_cache_new(block->cache);
    comp->blocks.blocks[comp->blocks.len++] = block;
    return block;
}

static vm_arg_t vm_ast_comp_reg(vm_ast_comp_t *comp) {
    size_t reg = ++comp->nregs;
    if (reg >= comp->names_alloc) {
        comp->names_alloc = reg * 2;
        comp->names = vm_realloc(comp->names, sizeof(const char *) * comp->names_alloc);
    }
    comp->names[reg] = NULL;
    return (vm_arg_t){
        .type = VM_ARG_REG,
        .reg = reg,
    };
}

static void vm_ast_blocks_instr(vm_ast_comp_t *comp, vm_instr_t instr) {
    vm_block_realloc(comp->cur, instr);
}

static void vm_ast_blocks_branch(vm_ast_comp_t *comp, vm_branch_t branch) {
    if (comp->cur->branch.op != VM_BOP_FALL) {
        __builtin_trap();
    }
    comp->cur->branch = branch;
}

static size_t vm_ast_comp_get_local(vm_ast_comp_t *comp, const char *name) {
    for (size_t i = 1; i < comp->nregs; i++) {
        if (!strcmp(comp->names[i], name)) {
            return i;
        }
    }
    return 0;
}

static vm_arg_t vm_ast_comp_to(vm_ast_comp_t *comp, vm_ast_node_t node) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            switch (form.type) {
                case VM_AST_FORM_DO: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    return arg2;
                }
                case VM_AST_FORM_SET: {
                    vm_arg_t value_arg = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_node_t target = form.args[0];
                    if (target.type == VM_AST_NODE_IDENT) {
                        size_t local = vm_ast_comp_get_local(comp, target.value.ident);
                        vm_ast_blocks_instr(
                            comp,
                            (vm_instr_t){
                                .op = VM_IOP_MOVE,
                                .out = local,
                                .args = vm_ast_args(1, value_arg),
                            }
                        );
                        return (vm_arg_t){
                            .type = VM_ARG_REG,
                            .reg = local,
                        };
                    }
                    break;
                }
                case VM_AST_FORM_ENV: {
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = VM_IOP_STD,
                            .out = out,
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_NEW: {
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = VM_IOP_NEW,
                            .out = out,
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_LOAD: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t) {
                            .op = VM_BOP_GET,
                            .out = out,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = next,
                        }
                    );
                    comp->cur = next;
                    return out;
                }
                case VM_AST_FORM_ADD:
                case VM_AST_FORM_SUB:
                case VM_AST_FORM_MUL:
                case VM_AST_FORM_DIV:
                case VM_AST_FORM_MOD: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    uint8_t op;
                    switch (form.type) {
                        case VM_AST_FORM_ADD: {
                            op = VM_IOP_ADD;
                            break;
                        }
                        case VM_AST_FORM_SUB: {
                            op = VM_IOP_SUB;
                            break;
                        }
                        case VM_AST_FORM_MUL: {
                            op = VM_IOP_MUL;
                            break;
                        }
                        case VM_AST_FORM_DIV: {
                            op = VM_IOP_DIV;
                            break;
                        }
                        case VM_AST_FORM_MOD: {
                            op = VM_IOP_MOD;
                            break;
                        }
                    }
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = op,
                            .args = vm_ast_args(2, arg1, arg2),
                            .out = out,
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_POW: {
                    break;
                }
                case VM_AST_FORM_EQ: {
                    break;
                }
                case VM_AST_FORM_NE: {
                    break;
                }
                case VM_AST_FORM_LT: {
                    break;
                }
                case VM_AST_FORM_GT: {
                    break;
                }
                case VM_AST_FORM_LE: {
                    break;
                }
                case VM_AST_FORM_GE: {
                    break;
                }
                case VM_AST_FORM_AND: {
                    break;
                }
                case VM_AST_FORM_OR: {
                    break;
                }
                case VM_AST_FORM_IF: {

                }
                case VM_AST_FORM_RETURN: {
                    vm_arg_t arg = vm_ast_comp_to(comp, form.args[0]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_RET,
                            .args = vm_ast_args(1, arg),
                        }
                    );
                    return (vm_arg_t){
                        .type = VM_ARG_NIL,
                    };
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_std_value_t num = node.value.literal;
            return (vm_arg_t){
                .type = VM_ARG_NUM,
                .num = num,
            };
        }
        case VM_AST_NODE_IDENT: {
            break;
        }
    }
    vm_ast_print_node(stdout, 0, "node = ", node);
    exit(1);
}

vm_ast_blocks_t vm_ast_comp(vm_ast_node_t node) {
    vm_ast_comp_t comp = (vm_ast_comp_t){
        .blocks = (vm_ast_blocks_t){
            .len = 0,
            .blocks = NULL,
            .alloc = 0,
        },
        .nregs = 0,
    };
    comp.cur = vm_ast_comp_new_block(&comp);
    vm_ast_comp_to(&comp, node);
    vm_block_info(comp.blocks.len, comp.blocks.blocks);
    return comp.blocks;
}
