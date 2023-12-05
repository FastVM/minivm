

#include "comp.h"
#include "../type.h"
#include "ast.h"
#include "build.h"
#include "print.h"

struct vm_ast_comp_t;
typedef struct vm_ast_comp_t vm_ast_comp_t;

struct vm_ast_comp_cap_t;
typedef struct vm_ast_comp_cap_t vm_ast_comp_cap_t;

struct vm_ast_comp_names_t;
typedef struct vm_ast_comp_names_t vm_ast_comp_names_t;

struct vm_ast_comp_t {
    vm_ast_blocks_t blocks;
    vm_block_t *cur;
    vm_ast_comp_names_t *names;
    bool is_error: 1;
};

struct vm_ast_comp_cap_t {
    const char *name;
    size_t slot;
};

struct vm_ast_comp_names_t {
    struct {
        size_t len;
        const char **ptr;
        size_t alloc;
    } regs;
    struct {
        size_t len;
        vm_ast_comp_cap_t *ptr;
        size_t alloc;
    } caps;
    vm_ast_comp_names_t *next;
    bool cap2;
};

static void vm_ast_comp_names_push(vm_ast_comp_t *comp) {
    vm_ast_comp_names_t *names = vm_malloc(sizeof(vm_ast_comp_names_t));
    *names = (vm_ast_comp_names_t){
        .next = comp->names,
    };
    comp->names = names;
}

static vm_ast_comp_names_t *vm_ast_comp_names_pop(vm_ast_comp_t *comp) {
    vm_ast_comp_names_t *old = comp->names;
    comp->names = comp->names->next;
    return old;
}

static vm_arg_t vm_ast_comp_to(vm_ast_comp_t *comp, vm_ast_node_t node);
static void vm_ast_comp_br(vm_ast_comp_t *comp, vm_ast_node_t node, vm_block_t *iftrue, vm_block_t *iffalse);

void vm_std_vm_closure(vm_std_value_t *args);

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
        // comp->blocks.alloc = (comp->blocks.len + 1) * 4;
        comp->blocks.alloc = (comp->blocks.len + 1) * 128;
        comp->blocks.blocks = vm_realloc(comp->blocks.blocks, sizeof(vm_block_t *) * comp->blocks.alloc);
    }
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t){
        .id = (ptrdiff_t)comp->blocks.len,
    };
    vm_cache_new(&block->cache);
    comp->blocks.blocks[comp->blocks.len++] = block;
    return block;
}

static vm_arg_t vm_ast_comp_reg_named(vm_ast_comp_t *comp, const char *name) {
    size_t reg = comp->names->regs.len++;
    if (reg + 1 >= comp->names->regs.alloc) {
        comp->names->regs.alloc = (reg + 1) * 2;
        comp->names->regs.ptr = vm_realloc(comp->names->regs.ptr, sizeof(const char *) * comp->names->regs.alloc);
    }
    comp->names->regs.ptr[reg] = name;
    return (vm_arg_t){
        .type = VM_ARG_REG,
        .reg = reg + 1,
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

static size_t vm_ast_comp_get_local(vm_ast_comp_names_t *names, const char *name) {
    for (size_t i = 0; i < names->regs.len; i++) {
        if (names->regs.ptr[i] != NULL && !strcmp(names->regs.ptr[i], name)) {
            return i + 1;
        }
    }
    return SIZE_MAX;
}

static vm_arg_t vm_ast_comp_names_get_nonlocal(vm_ast_comp_t *comp, vm_ast_comp_names_t *names, vm_arg_t cap_arg, const char *name) {
    for (size_t i = 0; i < names->caps.len; i++) {
        vm_ast_comp_cap_t cap = names->caps.ptr[i];
        if (!strcmp(cap.name, name)) {
            vm_arg_t reg = vm_ast_comp_reg(comp);
            vm_block_t *next = vm_ast_comp_new_block(comp);
            vm_arg_t slot = (vm_arg_t){
                .type = VM_ARG_LIT,
                .lit = (vm_std_value_t){
                    .tag = VM_TAG_I32,
                    .value = cap.slot,
                }
            };
            vm_ast_blocks_branch(
                comp,
                (vm_branch_t){
                    .op = VM_BOP_GET,
                    .out = reg,
                    .args = vm_ast_args(2, cap_arg, slot),
                    .targets[0] = next,
                }
            );
            comp->cur = next;
            return reg;
        }
    }
    if (names->next == NULL) {
        return (vm_arg_t){
            .type = VM_ARG_NONE,
        };
    }
    size_t slotnum = names->caps.len++;
    if (slotnum + 1 > names->caps.alloc) {
        names->caps.alloc = (slotnum + 1) * 2;
        names->caps.ptr = vm_realloc(names->caps.ptr, sizeof(vm_ast_comp_cap_t) * names->caps.alloc);
    }
    names->caps.ptr[slotnum] = (vm_ast_comp_cap_t){
        .name = name,
        .slot = slotnum + 1,
    };
    vm_arg_t reg = vm_ast_comp_reg(comp);
    vm_block_t *next = vm_ast_comp_new_block(comp);
    vm_arg_t slot = (vm_arg_t){
        .type = VM_ARG_LIT,
        .lit = (vm_std_value_t){
            .tag = VM_TAG_I32,
            .value = slotnum + 1,
        }
    };
    vm_ast_blocks_branch(
        comp,
        (vm_branch_t){
            .op = VM_BOP_GET,
            .out = reg,
            .args = vm_ast_args(2, cap_arg, slot),
            .targets[0] = next,
        }
    );
    comp->cur = next;
    return reg;
}

static vm_arg_t vm_ast_comp_get_var(vm_ast_comp_t *comp, const char *name) {
    size_t got = vm_ast_comp_get_local(comp->names, name);
    if (got != SIZE_MAX) {
        return (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = got,
        };
    };
    vm_arg_t cap = (vm_arg_t){
        .type = VM_ARG_REG,
        .reg = 0,
    };
    return vm_ast_comp_names_get_nonlocal(comp, comp->names, cap, name);
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
                case VM_AST_FORM_AND: {
                    vm_block_t *mid = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br(comp, form.args[0], mid, iffalse);
                    comp->cur = mid;
                    vm_ast_comp_br(comp, form.args[1], iftrue, iffalse);
                    return;
                }
                case VM_AST_FORM_OR: {
                    vm_block_t *mid = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br(comp, form.args[0], iftrue, mid);
                    comp->cur = mid;
                    vm_ast_comp_br(comp, form.args[1], iftrue, iffalse);
                    return;
                }
                case VM_AST_FORM_NOT: {
                    vm_ast_comp_br(comp, form.args[0], iffalse, iftrue);
                    return;
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_std_value_t value = node.value.literal;
            if (value.tag == VM_TAG_ERROR) {
                comp->is_error = true;
                return;
            }
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
    vm_arg_t bb_arg = vm_ast_comp_to(comp, node);
    vm_ast_blocks_branch(
        comp,
        (vm_branch_t){
            .op = VM_BOP_BB,
            .args = vm_ast_args(1, bb_arg),
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        }
    );
}

static vm_arg_t vm_ast_comp_to(vm_ast_comp_t *comp, vm_ast_node_t node) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            switch (form.type) {

                case VM_AST_FORM_EQ:
                case VM_AST_FORM_NE:
                case VM_AST_FORM_LT:
                case VM_AST_FORM_GT:
                case VM_AST_FORM_LE:
                case VM_AST_FORM_GE:
                case VM_AST_FORM_AND:
                case VM_AST_FORM_OR:
                case VM_AST_FORM_NOT: {
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br(comp, node, iftrue, iffalse);
                    comp->cur = iftrue;
                    vm_arg_t true_value = (vm_arg_t){
                        .type = VM_ARG_LIT,
                        .lit = (vm_std_value_t){
                            .value.b = true,
                            .tag = VM_TAG_BOOL,
                        },
                    };
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
                    vm_arg_t false_value = (vm_arg_t){
                        .type = VM_ARG_LIT,
                        .lit = (vm_std_value_t){
                            .value.b = false,
                            .tag = VM_TAG_BOOL,
                        },
                    };
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
                case VM_AST_FORM_DO: {
                    vm_ast_comp_to(comp, form.args[0]);
                    vm_arg_t arg2 = vm_ast_comp_to(comp, form.args[1]);
                    return arg2;
                }
                case VM_AST_FORM_LOCAL: {
                    vm_arg_t value_arg = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_node_t target = form.args[0];
                    size_t local = vm_ast_comp_reg_named(comp, target.value.ident).reg;
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
                    return vm_arg_nil();
                }
                case VM_AST_FORM_SET: {
                    vm_arg_t value_arg = vm_ast_comp_to(comp, form.args[1]);
                    vm_ast_node_t target = form.args[0];
                    if (target.type == VM_AST_NODE_IDENT) {
                        size_t local = vm_ast_comp_get_local(comp->names, target.value.ident);
                        if (local == SIZE_MAX) {
                            vm_arg_t env_arg = vm_ast_comp_to(comp, vm_ast_build_ident("_ENV"));
                            vm_arg_t key_arg = (vm_arg_t){
                                .type = VM_ARG_LIT,
                                .lit = (vm_std_value_t){
                                    .tag = VM_TAG_STR,
                                    .value.str = target.value.ident,
                                },
                            };
                            vm_ast_blocks_instr(
                                comp,
                                (vm_instr_t){
                                    .op = VM_IOP_SET,
                                    .out = (vm_arg_t){
                                        .type = VM_ARG_NONE,
                                    },
                                    .args = vm_ast_args(3, env_arg, key_arg, value_arg),
                                }
                            );
                            return vm_arg_nil();
                        } else {
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
                            return vm_arg_nil();
                        }
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
                        return vm_arg_nil();
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
                case VM_AST_FORM_LEN: {
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_arg_t in = vm_ast_comp_to(comp, form.args[0]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_instr_t){
                            .op = VM_IOP_LEN,
                            .args = vm_ast_args(1, in),
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
                case VM_AST_FORM_CONCAT: {
                    break;
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
                    vm_ast_comp_names_push(comp);

                    vm_block_t *old_cur = comp->cur;
                    vm_block_t *body = vm_ast_comp_new_block(comp);
                    comp->cur = body;

                    vm_ast_form_t args = form.args[1].value.form;

                    body->nargs = args.len + 1;
                    body->args = vm_malloc(sizeof(vm_arg_t) * body->nargs);
                    body->args[0] = (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = 0,
                    };
                    for (size_t i = 0; i < args.len; i++) {
                        vm_ast_node_t arg = args.args[i];
                        if (arg.type != VM_AST_NODE_IDENT) {
                            __builtin_trap();
                        }
                        body->args[i + 1] = vm_ast_comp_reg_named(comp, arg.value.ident);
                    }

                    if (form.args[0].type == VM_AST_NODE_IDENT) {
                        vm_arg_t out = vm_ast_comp_reg_named(comp, form.args[0].value.ident);
                        vm_arg_t cap = (vm_arg_t){
                            .type = VM_ARG_REG,
                            .reg = 0,
                        };
                        vm_ast_blocks_instr(
                            comp,
                            (vm_instr_t){
                                .op = VM_IOP_MOVE,
                                .out = out,
                                .args = vm_ast_args(1, cap),
                            }
                        );
                    }

                    vm_ast_comp_to(comp, form.args[2]);

                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_RET,
                            .args = vm_ast_args(1, vm_arg_nil()),
                        }
                    );

                    vm_ast_comp_names_t *names = vm_ast_comp_names_pop(comp);
                    comp->cur = old_cur;

                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *with_lambda = vm_ast_comp_new_block(comp);

                    vm_arg_t *call_args = vm_malloc(sizeof(vm_arg_t) * (names->caps.len + 3));
                    call_args[0] = (vm_arg_t){
                        .type = VM_ARG_LIT,
                        .lit = (vm_std_value_t){
                            .tag = VM_TAG_FFI,
                            .value.ffi = &vm_std_vm_closure,
                        },
                    };
                    call_args[1] = (vm_arg_t){
                        .type = VM_ARG_FUN,
                        .func = body,
                    };
                    for (size_t i = 0; i < names->caps.len; i++) {
                        vm_ast_comp_cap_t cap = names->caps.ptr[i];
                        vm_arg_t got = vm_ast_comp_get_var(comp, cap.name);
                        if (got.type != VM_ARG_NONE) {
                            call_args[i + 2] = got;
                        } else {
                            call_args[i + 2] = vm_arg_nil();
                        }
                    }
                    call_args[names->caps.len + 2] = (vm_arg_t){
                        .type = VM_ARG_NONE,
                    };

                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_CALL,
                            .out = out,
                            .args = call_args,
                            .targets[0] = with_lambda,
                        }
                    );

                    comp->cur = with_lambda;

                    return out;
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
                    vm_ast_comp_to(comp, form.args[1]);
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
                    return vm_arg_nil();
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
                    return vm_arg_nil();
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_std_value_t num = node.value.literal;
            if (num.tag == VM_TAG_NIL) {
                return vm_arg_nil();
            } else if (num.tag == VM_TAG_STR) {
                vm_arg_t str = (vm_arg_t){
                    .type = VM_ARG_LIT,
                    .lit = num,
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
            } else if (num.tag == VM_TAG_ERROR) {
                comp->is_error = true;
                return (vm_arg_t) {
                    .type = VM_ARG_NONE,
                };
            } else {
                return (vm_arg_t){
                    .type = VM_ARG_LIT,
                    .lit = num,
                };
            }
        }
        case VM_AST_NODE_IDENT: {
            const char *lit = node.value.ident;
            bool is_local = false;
            for (vm_ast_comp_names_t *names = comp->names; names != NULL && !is_local; names = names->next) {
                for (size_t i = 0; i < names->regs.len; i++) {
                    if (names->regs.ptr[i] != NULL && !strcmp(names->regs.ptr[i], lit)) {
                        is_local = true;
                        break;
                    }
                }
            }
            if (is_local) {
                vm_arg_t got = vm_ast_comp_get_var(comp, lit);
                if (got.type != VM_ARG_NONE) {
                    return got;
                }
            } else {
                vm_arg_t got = vm_ast_comp_get_var(comp, "_ENV");
                if (got.type != VM_ARG_NONE) {
                    vm_arg_t env_key = (vm_arg_t){
                        .type = VM_ARG_LIT,
                        .lit = (vm_std_value_t){
                            .tag = VM_TAG_STR,
                            .value.str = lit,
                        },
                    };
                    vm_arg_t out = vm_ast_comp_reg(comp);
                    vm_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_branch_t){
                            .op = VM_BOP_GET,
                            .out = out,
                            .args = vm_ast_args(2, got, env_key),
                            .targets[0] = next,
                        }
                    );
                    comp->cur = next;
                    return out;
                }
            }
        }
    }
    vm_ast_print_node(stdout, 0, "error node = ", node);
    exit(1);
}

void vm_ast_comp_more(vm_ast_node_t node, vm_ast_blocks_t *blocks) {
    vm_ast_comp_t comp = (vm_ast_comp_t){
        .blocks = *blocks,
        .names = NULL,
        .cur = NULL,
        .is_error = false,
    };
    for (size_t i = 0; i < blocks->len; i++) {
        vm_block_t *block = blocks->blocks[i];
        block->cache.len = 0;
    }
    vm_ast_comp_names_push(&comp);
    comp.blocks.entry = vm_ast_comp_new_block(&comp);
    size_t start = comp.blocks.entry->id;
    comp.cur = comp.blocks.entry; 
    vm_ast_blocks_instr(
        &comp,
        (vm_instr_t){
            .op = VM_IOP_STD,
            .out = vm_ast_comp_reg_named(&comp, "_ENV"),
            .args = vm_ast_args(0),
        }
    );
    vm_ast_comp_to(&comp, node);
    vm_ast_comp_names_pop(&comp);
    for (size_t i = start; i < comp.blocks.len; i++) {
        vm_block_t *block = comp.blocks.blocks[i];
        if (block->branch.op == VM_BOP_FALL) {
            block->branch.args = vm_ast_args(0);
        }
    }
    // vm_block_info(comp.blocks.len - start, comp.blocks.blocks + start);
    vm_block_info(comp.blocks.len, comp.blocks.blocks);
    *blocks = comp.blocks;
}

vm_ast_blocks_t vm_ast_comp(vm_ast_node_t node) {
    vm_ast_blocks_t blocks = (vm_ast_blocks_t) {
        .len = 0,
        .blocks = NULL,
        .alloc = 0,
    };
    vm_ast_comp_more(
        node,
        &blocks
    );
    return blocks;
}