
#include "comp.h"
#include "ast.h"
#include "build.h"
#include "print.h"
#include "../gc.h"

struct vm_ast_comp_t;
typedef struct vm_ast_comp_t vm_ast_comp_t;

struct vm_ast_comp_cap_t;
typedef struct vm_ast_comp_cap_t vm_ast_comp_cap_t;

struct vm_ast_comp_names_t;
typedef struct vm_ast_comp_names_t vm_ast_comp_names_t;

struct vm_ast_comp_t {
    vm_ir_block_t *cur;
    vm_ast_comp_names_t *names;
    vm_ir_block_t *on_break;
    vm_location_range_t range;
    vm_t *vm;
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

static void vm_ast_names_free(vm_ast_comp_names_t *names) {
    for (size_t i = 0; i < names->regs.len; i++) {
        vm_free(names->regs.ptr[i]);
    }
    vm_free(names->regs.ptr);
    for (size_t i = 0; i < names->caps.len; i++) {
        vm_free(names->caps.ptr[i].name);
    }
    vm_free(names->caps.ptr);
    vm_free(names);
}

static vm_ir_arg_t vm_ast_comp_to_raw(vm_ast_comp_t *comp, vm_ast_node_t node);
static vm_ir_arg_t vm_ast_comp_br_raw(vm_ast_comp_t *comp, vm_ast_node_t node, vm_ir_block_t *iftrue, vm_ir_block_t *iffalse);
#define vm_ast_comp_to_rec(comp, node) ({                                                        \
    vm_ast_comp_t *comp_ = comp;                                                                 \
    vm_ast_node_t node_ = node;                                                                  \
    vm_ir_arg_t ret = vm_ast_comp_to_raw(comp_, node_);                                             \
    if (ret.type == VM_IR_ARG_TYPE_ERROR && node_.info.range.start.byte != node_.info.range.stop.byte) { \
        return (vm_ir_arg_t){                                                                       \
            .type = VM_IR_ARG_TYPE_ERROR,                                                                \
            .error = vm_error_from_error(node_.info.range, ret.error),                           \
        };                                                                                       \
    }                                                                                            \
    ret;                                                                                         \
})

#define vm_ast_comp_br_rec(comp, node, iftrue, iffalse) ({                \
    vm_ast_comp_t *comp_ = (comp);                                        \
    vm_ast_node_t node_ = (node);                                         \
    vm_ir_arg_t ret = vm_ast_comp_br_raw(comp_, node_, (iftrue), (iffalse)); \
    if (ret.type == VM_IR_ARG_TYPE_ERROR) {                                       \
        return (vm_ir_arg_t){                                                \
            .type = VM_IR_ARG_TYPE_ERROR,                                         \
            .error = vm_error_from_error(node_.info.range, ret.error),                           \
        };                                                                \
    }                                                                     \
    ret;                                                                  \
})

extern void vm_std_vm_closure(vm_t *vm, size_t nargs, vm_obj_t *args);
extern void vm_std_vm_concat(vm_t *vm, size_t nargs, vm_obj_t *args);

static vm_ir_arg_t *vm_ast_args(size_t nargs, ...) {
    va_list ap;
    va_start(ap, nargs);
    vm_ir_arg_t *ret = vm_malloc(sizeof(vm_ir_arg_t) * (nargs + 1));
    for (size_t i = 0; i < nargs; i++) {
        ret[i] = va_arg(ap, vm_ir_arg_t);
    }
    va_end(ap);
    ret[nargs] = (vm_ir_arg_t){
        .type = VM_IR_ARG_TYPE_NONE,
    };
    return ret;
}

static vm_ir_block_t *vm_ast_comp_new_block(vm_ast_comp_t *comp) {
    vm_ir_block_t *block = vm_malloc(sizeof(vm_ir_block_t));
    *block = (vm_ir_block_t){
        .id = (uint32_t)comp->vm->nblocks++,
        // .nregs = VM_NREGS,
    };
    vm_gc_add(comp->vm, vm_obj_of_block(block));
    return block;
}

static vm_ir_arg_t vm_ast_comp_reg_named(vm_ast_comp_t *comp, const char *name) {
    size_t reg = comp->names->regs.len++;
    if (reg + 1 >= comp->names->regs.alloc) {
        comp->names->regs.alloc = (reg + 1) * 2;
        comp->names->regs.ptr = vm_realloc(comp->names->regs.ptr, sizeof(const char *) * comp->names->regs.alloc);
    }
    if (name == NULL) {
        comp->names->regs.ptr[reg] = NULL;
    } else {
        comp->names->regs.ptr[reg] = vm_strdup(name);
    }
    return (vm_ir_arg_t){
        .type = VM_IR_ARG_TYPE_REG,
        .reg = reg,
    };
}

static vm_ir_arg_t vm_ast_comp_reg(vm_ast_comp_t *comp) {
    return vm_ast_comp_reg_named(comp, NULL);
}

static void vm_ast_blocks_instr(vm_ast_comp_t *comp, vm_ir_instr_t instr) {
    instr.range = comp->range;
    vm_block_realloc(comp->cur, instr);
}

static void vm_ast_blocks_branch(vm_ast_comp_t *comp, vm_ir_branch_t branch) {
    branch.range = comp->range;
    if (comp->cur->branch.op != VM_IR_BRANCH_OPCODE_FALL) {
        __builtin_trap();
    }
    comp->cur->branch = branch;
}

static size_t vm_ast_comp_get_local(vm_ast_comp_names_t *names, const char *name) {
    size_t i = names->regs.len;
    while (i > 0) {
        i -= 1;
        if (names->regs.ptr[i] != NULL && !strcmp(names->regs.ptr[i], name)) {
            return i;
        }
    }
    return SIZE_MAX;
}

static vm_ir_arg_t vm_ast_comp_get_var(vm_ast_comp_t *comp, const char *name) {
    if (name == NULL) {
        __builtin_trap();
    }
    size_t got = vm_ast_comp_get_local(comp->names, name);
    if (got != SIZE_MAX) {
        return (vm_ir_arg_t){
            .type = VM_IR_ARG_TYPE_REG,
            .reg = got,
        };
    };
    vm_ir_arg_t cap = (vm_ir_arg_t){
        .type = VM_IR_ARG_TYPE_REG,
        .reg = 0,
    };
    ptrdiff_t slotnum = -1;
    for (size_t i = 0; i < comp->names->caps.len; i++) {
        vm_ast_comp_cap_t cap = comp->names->caps.ptr[i];
        if (!strcmp(cap.name, name)) {
            slotnum = cap.slot;
            break;
        }
    }
    if (slotnum < 0) {
        slotnum = ++comp->names->caps.len;
        if (slotnum + 1 >= (ptrdiff_t)comp->names->caps.alloc) {
            comp->names->caps.alloc = slotnum * 2;
            comp->names->caps.ptr = vm_realloc(comp->names->caps.ptr, sizeof(vm_ast_comp_cap_t) * comp->names->caps.alloc);
        }
        comp->names->caps.ptr[slotnum - 1] = (vm_ast_comp_cap_t){
            .name = vm_strdup(name),
            .slot = slotnum,
        };
    }
    vm_ir_arg_t reg = vm_ast_comp_reg(comp);
    vm_ir_block_t *next = vm_ast_comp_new_block(comp);
    vm_ir_arg_t slot = (vm_ir_arg_t){
        .type = VM_IR_ARG_TYPE_LIT,
        .lit = vm_obj_of_number(slotnum - 1),
    };
    vm_ast_blocks_branch(
        comp,
        (vm_ir_branch_t){
            .op = VM_IR_BRANCH_OPCODE_LOAD,
            .out = reg,
            .args = vm_ast_args(2, cap, slot),
            .targets[0] = next,
        }
    );
    comp->cur = next;
    return reg;
}

static vm_ir_arg_t vm_ast_comp_br_raw(vm_ast_comp_t *comp, vm_ast_node_t node, vm_ir_block_t *iftrue, vm_ir_block_t *iffalse) {
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            switch (form.type) {
                case VM_AST_FORM_LT: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BLT,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_GT: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BLT,
                            .args = vm_ast_args(2, arg2, arg1),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_LE: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BLE,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_GE: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BLE,
                            .args = vm_ast_args(2, arg2, arg1),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_EQ: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BEQ,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_NE: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BEQ,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = iffalse,
                            .targets[1] = iftrue,
                        }
                    );
                    goto ret_ok;
                }
                case VM_AST_FORM_AND: {
                    vm_ir_block_t *mid = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br_rec(comp, form.args[0], mid, iffalse);
                    comp->cur = mid;
                    vm_ast_comp_br_rec(comp, form.args[1], iftrue, iffalse);
                    goto ret_ok;
                }
                case VM_AST_FORM_OR: {
                    vm_ir_block_t *mid = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br_rec(comp, form.args[0], iftrue, mid);
                    comp->cur = mid;
                    vm_ast_comp_br_rec(comp, form.args[1], iftrue, iffalse);
                    goto ret_ok;
                }
                case VM_AST_FORM_NOT: {
                    vm_ast_comp_br_rec(comp, form.args[0], iffalse, iftrue);
                    goto ret_ok;
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_obj_t value = node.value.literal;
            if (vm_obj_is_error(value)) {
                return (vm_ir_arg_t){
                    .type = VM_IR_ARG_TYPE_ERROR,
                    .error = vm_obj_get_error(value),
                };
            }
            vm_ast_blocks_branch(
                comp,
                (vm_ir_branch_t){
                    .op = VM_IR_BRANCH_OPCODE_JUMP,
                    .args = vm_ast_args(0),
                    .targets[0] = iftrue,
                }
            );
            goto ret_ok;
            ;
        }
        case VM_AST_NODE_IDENT: {
            break;
        }
    }
    vm_ir_arg_t bb_arg = vm_ast_comp_to_rec(comp, node);
    vm_ast_blocks_branch(
        comp,
        (vm_ir_branch_t){
            .op = VM_IR_BRANCH_OPCODE_BB,
            .args = vm_ast_args(1, bb_arg),
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        }
    );
    goto ret_ok;
ret_ok:;
    return (vm_ir_arg_t){.type = VM_IR_ARG_TYPE_NONE};
}

static vm_ir_arg_t vm_ast_comp_to_raw(vm_ast_comp_t *comp, vm_ast_node_t node) {
    // {
    //     vm_io_buffer_t *buf = vm_io_buffer_new();
    //     vm_ast_print_node(buf, 0, "ast = ", node);
    //     printf("%s\n", buf->buf);
    // }
    if (node.info.range.start.byte != node.info.range.stop.byte) {
        comp->range = node.info.range;
    }
    switch (node.type) {
        case VM_AST_NODE_FORM: {
            vm_ast_form_t form = node.value.form;
            switch (form.type) {
                case VM_AST_FORM_AND: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BB,
                            .args = vm_ast_args(1, arg1),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    comp->cur = iffalse;
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, arg1),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = iftrue;
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, arg2),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = after;
                    return out;
                }
                case VM_AST_FORM_OR: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_BB,
                            .args = vm_ast_args(1, arg1),
                            .targets[0] = iftrue,
                            .targets[1] = iffalse,
                        }
                    );
                    comp->cur = iftrue;
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, arg1),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = iffalse;
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, arg2),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = after;
                    return out;
                }
                case VM_AST_FORM_EQ:
                case VM_AST_FORM_NE:
                case VM_AST_FORM_LT:
                case VM_AST_FORM_GT:
                case VM_AST_FORM_LE:
                case VM_AST_FORM_GE:
                case VM_AST_FORM_NOT: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br_rec(comp, node, iftrue, iffalse);
                    comp->cur = iftrue;
                    vm_ir_arg_t true_value = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_LIT,
                        .lit = vm_obj_of_boolean(true),
                    };
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, true_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = iffalse;
                    vm_ir_arg_t false_value = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_LIT,
                        .lit = vm_obj_of_boolean(false),
                    };
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, false_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = after;
                    return out;
                }
                case VM_AST_FORM_DO: {
                    vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    return arg2;
                }
                case VM_AST_FORM_LOCAL: {
                    vm_ir_arg_t value_arg = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_node_t target = form.args[0];
                    vm_ir_arg_t local = vm_ast_comp_reg_named(comp, target.value.ident);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = local,
                            .args = vm_ast_args(1, value_arg),
                        }
                    );
                    return vm_arg_nil();
                }
                case VM_AST_FORM_SET: {
                    vm_ir_arg_t value_arg = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_node_t target = form.args[0];
                    if (target.type == VM_AST_NODE_IDENT) {
                        size_t local = vm_ast_comp_get_local(comp->names, target.value.ident);
                        if (local == SIZE_MAX) {
                            vm_ast_node_t node = vm_ast_build_ident(vm_strdup("_ENV"));
                            vm_ir_arg_t env_arg = vm_ast_comp_to_rec(comp, node);
                            vm_ast_free_node(node);
                            vm_ir_arg_t key_arg = (vm_ir_arg_t){
                                .type = VM_IR_ARG_TYPE_LIT,
                                .lit = vm_obj_of_string(comp->vm, target.value.ident),
                            };
                            vm_ast_blocks_instr(
                                comp,
                                (vm_ir_instr_t){
                                    .op = VM_IR_INSTR_OPCODE_TABLE_SET,
                                    .out = (vm_ir_arg_t){
                                        .type = VM_IR_ARG_TYPE_NONE,
                                    },
                                    .args = vm_ast_args(3, env_arg, key_arg, value_arg),
                                }
                            );
                            return vm_arg_nil();
                        } else {
                            vm_ast_blocks_instr(
                                comp,
                                (vm_ir_instr_t){
                                    .op = VM_IR_INSTR_OPCODE_MOVE,
                                    .out = (vm_ir_arg_t){
                                        .type = VM_IR_ARG_TYPE_REG,
                                        .reg = local,
                                    },
                                    .args = vm_ast_args(1, value_arg),
                                }
                            );
                            return vm_arg_nil();
                        }
                    }
                    if (target.type == VM_AST_NODE_FORM && target.value.form.type == VM_AST_FORM_LOAD) {
                        vm_ir_arg_t table = vm_ast_comp_to_rec(comp, target.value.form.args[0]);
                        vm_ir_arg_t key = vm_ast_comp_to_rec(comp, target.value.form.args[1]);
                        vm_ir_arg_t val = vm_ast_comp_to_rec(comp, form.args[1]);
                        vm_ast_blocks_instr(
                            comp,
                            (vm_ir_instr_t){
                                .op = VM_IR_INSTR_OPCODE_TABLE_SET,
                                .args = vm_ast_args(3, table, key, val),
                                .out = (vm_ir_arg_t){
                                    .type = VM_IR_ARG_TYPE_NONE,
                                },
                            }
                        );
                        return vm_arg_nil();
                    }
                    break;
                }
                case VM_AST_FORM_ENV: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_STD,
                            .out = out,
                            .args = vm_ast_args(0),
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_NEW: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_TABLE_NEW,
                            .out = out,
                            .args = vm_ast_args(0),
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_LEN: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_arg_t in = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_TABLE_LEN,
                            .args = vm_ast_args(1, in),
                            .out = out,
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_LOAD: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_GET,
                            .out = out,
                            .args = vm_ast_args(2, arg1, arg2),
                            .targets[0] = next,
                        }
                    );
                    comp->cur = next;
                    return out;
                }
                case VM_AST_FORM_CALL: {
                    vm_ir_arg_t func = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t *args = vm_malloc(sizeof(vm_ir_arg_t) * (form.len + 1));
                    args[0] = func;
                    for (size_t i = 1; i < form.len; i++) {
                        args[i] = vm_ast_comp_to_rec(comp, form.args[i]);
                    }
                    args[form.len] = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_NONE,
                    };
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_CALL,
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
                case VM_AST_FORM_IDIV:
                case VM_AST_FORM_MOD:
                case VM_AST_FORM_POW:
                case VM_AST_FORM_CONCAT: {
                    vm_ir_arg_t arg1 = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ir_arg_t arg2 = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    uint8_t op;
                    switch (form.type) {
                        case VM_AST_FORM_ADD: {
                            op = VM_IR_INSTR_OPCODE_ADD;
                            break;
                        }
                        case VM_AST_FORM_SUB: {
                            op = VM_IR_INSTR_OPCODE_SUB;
                            break;
                        }
                        case VM_AST_FORM_MUL: {
                            op = VM_IR_INSTR_OPCODE_MUL;
                            break;
                        }
                        case VM_AST_FORM_DIV: {
                            op = VM_IR_INSTR_OPCODE_DIV;
                            break;
                        }
                        case VM_AST_FORM_IDIV: {
                            op = VM_IR_INSTR_OPCODE_IDIV;
                            break;
                        }
                        case VM_AST_FORM_MOD: {
                            op = VM_IR_INSTR_OPCODE_MOD;
                            break;
                        }
                        case VM_AST_FORM_POW: {
                            op = VM_IR_INSTR_OPCODE_POW;
                            break;
                        }
                        case VM_AST_FORM_CONCAT: {
                            op = VM_AST_FORM_CONCAT;
                            break;
                        }
                    }
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = op,
                            .args = vm_ast_args(2, arg1, arg2),
                            .out = out,
                        }
                    );
                    return out;
                }
                case VM_AST_FORM_IF: {
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *iftrue = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *iffalse = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br_rec(comp, form.args[0], iftrue, iffalse);
                    comp->cur = iftrue;
                    vm_ir_arg_t true_value = vm_ast_comp_to_rec(comp, form.args[1]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, true_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = iffalse;
                    vm_ir_arg_t false_value = vm_ast_comp_to_rec(comp, form.args[2]);
                    vm_ast_blocks_instr(
                        comp,
                        (vm_ir_instr_t){
                            .op = VM_IR_INSTR_OPCODE_MOVE,
                            .out = out,
                            .args = vm_ast_args(1, false_value),
                        }
                    );
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = after,
                        }
                    );
                    comp->cur = after;
                    return out;
                }
                case VM_AST_FORM_LAMBDA: {
                    vm_ast_comp_names_push(comp);

                    vm_ir_block_t *old_cur = comp->cur;
                    vm_ir_block_t *body = vm_ast_comp_new_block(comp);
                    comp->cur = body;
                    body->isfunc = true;

                    vm_ast_form_t args = form.args[1].value.form;

                    // reserve args
                    if (form.args[0].type == VM_AST_NODE_IDENT) {
                        vm_ast_comp_reg_named(comp, form.args[0].value.ident);
                    } else {
                        vm_ast_comp_reg_named(comp, NULL);
                    }
                    for (size_t i = 0; i < args.len; i++) {
                        vm_ast_node_t arg = args.args[i];
                        if (arg.type != VM_AST_NODE_IDENT) {
                            __builtin_trap();
                        }
                        vm_ast_comp_reg_named(comp, arg.value.ident);
                    }

                    vm_ir_block_t *old_break = comp->on_break;
                    comp->on_break = NULL;
                    vm_ast_comp_to_rec(comp, form.args[2]);
                    comp->on_break = old_break;

                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_RET,
                            .args = vm_ast_args(1, vm_arg_nil()),
                        }
                    );

                    vm_ast_comp_names_t *names = vm_ast_comp_names_pop(comp);
                    comp->cur = old_cur;

                    vm_ir_arg_t out = vm_ast_comp_reg(comp);

                    vm_ir_block_t *with_lambda = vm_ast_comp_new_block(comp);

                    vm_ir_arg_t *call_args = vm_malloc(sizeof(vm_ir_arg_t) * (names->caps.len + 3));
                    call_args[0] = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_LIT,
                        .lit = vm_obj_of_ffi(&vm_std_vm_closure),
                    };
                    call_args[1] = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_LIT,
                        .lit = vm_obj_of_block(body),
                    };
                    for (size_t i = 0; i < names->caps.len; i++) {
                        vm_ast_comp_cap_t cap = names->caps.ptr[i];
                        vm_ir_arg_t got = vm_ast_comp_get_var(comp, cap.name);
                        if (got.type != VM_IR_ARG_TYPE_NONE) {
                            call_args[i + 2] = got;
                        } else {
                            call_args[i + 2] = vm_arg_nil();
                        }
                    }
                    call_args[names->caps.len + 2] = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_NONE,
                    };

                    vm_ast_names_free(names);

                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_CALL,
                            .out = out,
                            .args = call_args,
                            .targets[0] = with_lambda,
                        }
                    );

                    comp->cur = with_lambda;

                    return out;
                }
                case VM_AST_FORM_WHILE: {
                    vm_ir_block_t *body = vm_ast_comp_new_block(comp);
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_comp_br_rec(comp, form.args[0], body, after);
                    comp->cur = body;
                    vm_ir_block_t *old_break = comp->on_break;
                    comp->on_break = after;
                    vm_ast_comp_to_rec(comp, form.args[1]);
                    comp->on_break = old_break;
                    vm_ast_comp_br_rec(comp, form.args[0], body, after);
                    comp->cur = after;
                    return vm_arg_nil();
                }
                case VM_AST_FORM_BREAK: {
                    if (comp->on_break == NULL) {
                        return (vm_ir_arg_t){
                            .type = VM_IR_ARG_TYPE_ERROR,
                            .error = vm_error_from_msg(vm_location_range_unknown, "break not in block"),
                        };
                    }
                    vm_ir_block_t *after = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_JUMP,
                            .args = vm_ast_args(0),
                            .targets[0] = comp->on_break,
                        }
                    );
                    comp->cur = after;
                    return vm_arg_nil();
                }
                case VM_AST_FORM_RETURN: {
                    vm_ir_arg_t arg = vm_ast_comp_to_rec(comp, form.args[0]);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_RET,
                            .args = vm_ast_args(1, arg),
                        }
                    );
                    comp->cur = vm_ast_comp_new_block(comp);
                    return vm_arg_nil();
                }
                case VM_AST_FORM_SCOPE: {
                    size_t count = comp->names->regs.len;
                    vm_ir_arg_t ret = vm_ast_comp_to_rec(comp, form.args[0]);
                    for (size_t i = count; i < comp->names->regs.len; i++) {
                        vm_free(comp->names->regs.ptr[i]);
                    }
                    comp->names->regs.len = count;
                    return ret;
                }
            }
            break;
        }
        case VM_AST_NODE_LITERAL: {
            vm_obj_t num = node.value.literal;
            if (vm_obj_is_error(num)) {
                return (vm_ir_arg_t){
                    .type = VM_IR_ARG_TYPE_ERROR,
                    .error = vm_obj_get_error(num),
                };
            } else {
                return (vm_ir_arg_t){
                    .type = VM_IR_ARG_TYPE_LIT,
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
                vm_ir_arg_t got = vm_ast_comp_get_var(comp, lit);
                if (got.type != VM_IR_ARG_TYPE_NONE) {
                    return got;
                }
            } else {
                vm_ir_arg_t got = vm_ast_comp_get_var(comp, "_ENV");
                if (got.type != VM_IR_ARG_TYPE_NONE) {
                    vm_ir_arg_t env_key = (vm_ir_arg_t){
                        .type = VM_IR_ARG_TYPE_LIT,
                        .lit = vm_obj_of_string(comp->vm, lit),
                    };
                    vm_ir_arg_t out = vm_ast_comp_reg(comp);
                    vm_ir_block_t *next = vm_ast_comp_new_block(comp);
                    vm_ast_blocks_branch(
                        comp,
                        (vm_ir_branch_t){
                            .op = VM_IR_BRANCH_OPCODE_GET,
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
    return (vm_ir_arg_t){
        .type = VM_IR_ARG_TYPE_ERROR,
        .error = vm_error_from_msg(node.info.range, "internal error: invalid or unhandled ast node type"),
    };
}

vm_ir_block_t *vm_ast_comp_more(vm_t *vm, vm_ast_node_t node) {
    vm_ast_comp_t comp = (vm_ast_comp_t){
        .names = NULL,
        .cur = NULL,
        .on_break = NULL,
        .vm = vm,
    };
    vm_ast_comp_names_push(&comp);
    vm_ir_block_t *entry = vm_ast_comp_new_block(&comp);
    size_t start = entry->id;
    comp.cur = entry;
    vm_ast_blocks_instr(
        &comp,
        (vm_ir_instr_t){
            .op = VM_IR_INSTR_OPCODE_STD,
            .out = vm_ast_comp_reg_named(&comp, "_ENV"),
            .args = vm_ast_args(0),
        }
    );
    vm_ir_arg_t result_arg = vm_ast_comp_to_raw(&comp, node);
    if (result_arg.type == VM_IR_ARG_TYPE_ERROR) {
        for (vm_error_t *error = result_arg.error; error != NULL; error = error->child) {
            if (error->child != NULL) {
                fprintf(stderr, "range: %zu .. %zu\n", error->range.start.byte, error->range.stop.byte);
            } else {
                fprintf(stderr, "error: %s\n", error->msg);
                break;
            }
        }
    }
    vm_ast_comp_names_t *names = vm_ast_comp_names_pop(&comp);
    vm_ast_names_free(names);
    return entry;
}
