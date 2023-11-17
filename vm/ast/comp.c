

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

static vm_arg_t vm_ast_comp_to(vm_ast_comp_t *comp, vm_ast_node_t node);
static void vm_ast_comp_br(vm_ast_comp_t *comp, vm_ast_node_t node, vm_block_t *iftrue, vm_block_t *iffalse);

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
    // if (block->id == 1) {
    //     __builtin_trap();
    // }
    vm_cache_new(block->cache);
    comp->blocks.blocks[comp->blocks.len++] = block;
    return block;
}

static vm_arg_t vm_ast_comp_reg_named(vm_ast_comp_t *comp, const char *name) {
    size_t reg = comp->nregs++;
    if (reg + 1 >= comp->names_alloc) {
        comp->names_alloc = (reg + 1) * 2;
        comp->names = vm_realloc(comp->names, sizeof(const char *) * comp->names_alloc);
    }
    comp->names[reg] = name;
    return (vm_arg_t){
        .type = VM_ARG_REG,
        .reg = reg,
    };
}

static vm_arg_t vm_ast_comp_reg(vm_ast_comp_t *comp) {
    return vm_ast_comp_reg_named(comp, NULL);
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
    for (size_t i = 0; i < comp->nregs; i++) {
        if (comp->names[i] != NULL && !strcmp(comp->names[i], name)) {
            return i;
        }
    }
    return SIZE_MAX;
}

static void vm_ast_comp_br(vm_ast_comp_t *comp, vm_ast_node_t node, vm_block_t *iftrue, vm_block_t *iffalse) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            switch (form.type) {
                case VM_AST_FORM_LT: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BLT,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    return;
                }
                case VM_AST_FORM_GT: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BLT,
                            .args = vm_ast_args(2, arg2, arg1),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    return;
                }
                case VM_AST_FORM_LE: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BLT,
                            .args = vm_ast_args(2, arg2, arg1),
                            .targets[0] = iffalse,
                            .targets[1] = iftrue,
                        }
                    );
                    return;
                }
                case VM_AST_FORM_GE: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BLT,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iffalse,
                            .targets[1] = iftrue,
                        }
                    );
                    return;
                }
                case VM_AST_FORM_EQ: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BEQ,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    return;
                }
                case VM_AST_FORM_NE: {
                    vm_arg_t arg1 = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_BEQ,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iffalse,
                            .targets[1] = iftrue,
                        }
                    );
                    return;
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_ast_blocks_branch(
                comp,
                (vm_branch_t){
                    .op = VM_BOP_JUMP,
                    .args = vm_ast_args(0),
                    .targets[0] = iftrue,
                }
            );
            return;
        }
        case VM_AST_NODE_IDENT: {
            break;
        }
    }
    vm_ast_print_node(stdout, 0, "error branch node = ", node);
    exit(1);
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
                        if (local == SIZE_MAX) {
                            local = vm_ast_comp_reg_named(comp, target.value.ident).reg;
                        }
                        vm_ast_blocks_instr(
                            comp,
                            (vm_instr_t){
                                .op = VM_IOP_MOVE,
                                .out = (vm_arg_t){
                                    .type = VM_ARG_REG,
                                    .reg = local,
                                },
                                .args = vm_ast_args(1, value_arg),
                            }
                        );
                        return (vm_arg_t){
                            .type = VM_ARG_REG,
                            .reg = local,
                        };
                    }
                    if (target.type == VM_AST_NODE_FORM && target.value.form.type == VM_AST_FORM_LOAD) {
                        vm_arg_t table = vm_ast_comp_to(comp, target.value.form.args[0]);
                        vm_arg_t key = vm_ast_comp_to(comp, target.value.form.args[1]);
                        vm_arg_t val = vm_ast_comp_to(comp, form.args[1]);
                        vm_ast_blocks_instr(
                            comp,
                            (vm_instr_t){
                                .op = VM_IOP_SET,
                                .args = vm_ast_args(3, table, key, val),
                                .out = (vm_arg_t){
                                    .type = VM_ARG_NONE,
                                },
                            }
                        );
                        return val;
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
                            .args = vm_ast_args(0),
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
                            .args = vm_ast_args(0),
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
                        (vm_branch_t){
                            .op = VM_BOP_GET,
                            .out = out,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = next,
                        }
                    );
                    comp->cur = next;
                    return out;
                }
                case VM_AST_FORM_CALL: {
                    vm_arg_t func = vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t *args = vm_malloc(sizeof(vm_arg_t) * (form.len + 1));
                    args[0] = func;
                    for (size_t i = 1; i < form.len; i++) {
                        args[i] = vm_ast_comp_to(comp, form.args[i]);
                    }
                    args[form.len] = (vm_arg_t){
                        .type = VM_ARG_NONE,
                    };
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_CALL,
                            .out = out,
                            .args = args,
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
                case VM_AST_FORM_IF: {
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br(comp, form.args[0], iftrue, iffalse);
                    comp->cur = iftrue;
                    vm_arg_t true_value = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = VM_IOP_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, true_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = iffalse;
                    vm_arg_t false_value = vm_ast_comp_to(comp, form.args[2]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = VM_IOP_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, false_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = after;
                    return out;
                }
                case VM_AST_FORM_LAMBDA: {
                    size_t old_nregs = comp->nregs;
                    const char **old_names = comp->names;
                    size_t old_names_alloc = comp->names_alloc;
                    vm_block_t *old_cur = comp->cur;
                    
                    vm_block_t *body = vm_ast_comp_new_block(comp);
                    comp->cur = body;

                    vm_ast_form_t args = form.args[0].value.form;

                    body->nargs = args.len;
                    body->args = vm_malloc(sizeof(vm_arg_t) * body->nargs);
                    for (size_t i = 0; i < args.len; i++) {
                        vm_ast_node_t arg = args.args[i];
                        if (arg.type != VM_AST_NODE_IDENT) {
                            __builtin_trap();
                        }
                        body->args[i] = vm_ast_comp_reg_named(comp, arg.value.ident);
                    }

                    vm_ast_comp_to(comp, form.args[1]);

                    comp->nregs = old_nregs;
                    comp->names = old_names;
                    comp->names_alloc = old_names_alloc;
                    comp->cur = old_cur;
                    return (vm_arg_t) {
                        .type = VM_ARG_FUNC,
                        .func = body,
                    };
                }
                case VM_AST_FORM_WHILE: {
                    // vm_block_t *cond = vm_ast_comp_new_block(comp);
                    vm_block_t *body = vm_ast_comp_new_block(comp);
                    vm_block_t *after = vm_ast_comp_new_block(comp);
                    // vm_ast_blocks_branch(
                    //     comp,
                    //     (vm_branch_t){
                    //         .op = VM_BOP_JUMP,
                    //         .args = vm_ast_args(0),
                    //         .targets[0] = cond,
                    //     }
                    // );
                    // comp->cur = cond;
                    vm_ast_comp_br(comp, form.args[0], body, after);
                    comp->cur = body;
                    vm_arg_t true_value = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_comp_br(comp, form.args[0], body, after);
                    // vm_ast_blocks_branch(
                    //     comp,
                    //     (vm_branch_t){
                    //         .op = VM_BOP_JUMP,
                    //         .args = vm_ast_args(0),
                    //         .targets[0] = cond,
                    //     }
                    // );
                    comp->cur = after;
                    return (vm_arg_t){
                        .type = VM_ARG_NIL,
                    };
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
                    comp->cur = vm_ast_comp_new_block(comp);
                    return (vm_arg_t){
                        .type = VM_ARG_NIL,
                    };
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_std_value_t num = node.value.literal;
            if (num.tag == VM_TAG_NIL) {
                return (vm_arg_t){
                    .type = VM_ARG_NIL,
                    .num = num,
                };
            } else if (num.tag == VM_TAG_NIL) {
                return (vm_arg_t){
                    .type = VM_ARG_NIL,
                };
            } else if (num.tag == VM_TAG_STR) {
                vm_arg_t str = (vm_arg_t){
                    .type = VM_ARG_STR,
                    .str = num.value.str,
                };
                vm_arg_t ret = vm_ast_comp_reg(comp);
                vm_ast_blocks_instr(
                    comp,
                    (vm_instr_t){
                        .op = VM_IOP_MOVE,
                        .out = ret,
                        .args = vm_ast_args(1, str),
                    }
                );
                return ret;
            } else {
                return (vm_arg_t){
                    .type = VM_ARG_NUM,
                    .num = num,
                };
            }
        }
        case VM_AST_NODE_IDENT: {
            const char *lit = node.value.ident;
            size_t got = vm_ast_comp_get_local(comp, lit);
            if (got == SIZE_MAX) {
                vm_arg_t env_table = vm_ast_comp_reg(comp);
                vm_ast_blocks_instr(
                    comp,
                    (vm_instr_t){
                        .op = VM_IOP_STD,
                        .out = env_table,
                        .args = vm_ast_args(0),
                    }
                );
                vm_arg_t env_key = (vm_arg_t){
                    .type = VM_ARG_STR,
                    .str = lit,
                };
                vm_arg_t out = vm_ast_comp_reg(comp);
                vm_block_t *next = vm_ast_comp_new_block(comp);
                vm_ast_blocks_branch(
                    comp,
                    (vm_branch_t){
                        .op = VM_BOP_GET,
                        .out = out,
                        .args = vm_ast_args(2, env_table, env_key),
                        .targets[0] = next,
                    }
                );
                comp->cur = next;
                return out;
            }
            return (vm_arg_t){
                .type = VM_ARG_REG,
                .reg = got,
            };
        }
    }
    vm_ast_print_node(stdout, 0, "error node = ", node);
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
    size_t write = 0;
    for (size_t i = 0; i < comp.blocks.len; i++) {
        vm_block_t *block = comp.blocks.blocks[i];
        if (block->branch.op == VM_BOP_FALL) {
            continue;
        }
        comp.blocks.blocks[write++] = block;
    }
    comp.blocks.len = write;
    vm_block_info(comp.blocks.len, comp.blocks.blocks);
    return comp.blocks;
}
