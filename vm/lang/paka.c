#include "paka.h"

enum {
    VM_PAKA_END,
    VM_PAKA_ELSE,
    VM_PAKA_ELSEIF,
};

bool vm_paka_parser_is_ident0_char(char c) {
    return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || c == '_';
}

bool vm_paka_parser_is_ident1_char(char c) {
    return vm_paka_parser_is_ident0_char(c) || ('0' <= c && c <= '9');
}

static bool vm_paka_parser_is_space_char(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void vm_paka_parser_skip(vm_paka_parser_t *parser) {
    char got = vm_paka_parser_peek(parser);
    if (got == '\n') {
        parser->line += 1;
        parser->col = 1;
    } else {
        parser->col += 1;
    }
    parser->index += 1;
}

char vm_paka_parser_peek(vm_paka_parser_t *parser) {
    return parser->src[parser->index];
}

char vm_paka_parser_read(vm_paka_parser_t *parser) {
    char ret = vm_paka_parser_peek(parser);
    vm_paka_parser_skip(parser);
    return ret;
}

size_t vm_paka_parser_tell(vm_paka_parser_t *parser) { return parser->index; }

bool vm_paka_parser_match(vm_paka_parser_t *parser, const char *str) {
    vm_paka_parser_t save = *parser;
    for (const char *c = str; *c != '\0'; c++) {
        if (*c != vm_paka_parser_read(parser)) {
            *parser = save;
            return false;
        }
    }
    return true;
}

bool vm_paka_parser_match_if(vm_paka_parser_t *parser, bool (*fn)(char)) {
    if (!fn(vm_paka_parser_peek(parser))) {
        return false;
    }
    vm_paka_parser_skip(parser);
    return true;
}

static bool vm_paka_parser_match_comment(vm_paka_parser_t *parser) {
    if (vm_paka_parser_match(parser, "--")) {
        while (vm_paka_parser_read(parser) != '\n') {
        }
        return true;
    }
    return false;
}

void vm_paka_parser_strip_spaces(vm_paka_parser_t *parser) {
    while (vm_paka_parser_match_if(parser, vm_paka_parser_is_space_char) || vm_paka_parser_match_comment(parser)) {
    }
}

size_t vm_paka_parser_ident_len(vm_paka_parser_t *parser) {
    vm_paka_parser_t save = *parser;
    size_t len = 0;
    if (vm_paka_parser_match_if(parser, vm_paka_parser_is_ident0_char)) {
        len += 1;
    }
    while (vm_paka_parser_match_if(parser, vm_paka_parser_is_ident1_char)) {
        len += 1;
    }
    *parser = save;
    return len;
}

bool vm_paka_parser_match_keyword(vm_paka_parser_t *parser,
                                  const char *keyword) {
    vm_paka_parser_t save = *parser;
    size_t start = vm_paka_parser_tell(parser);
    if (!vm_paka_parser_match(parser, keyword)) {
        return false;
    }
    size_t end = vm_paka_parser_tell(parser);
    if (end - start != vm_paka_parser_ident_len(&save)) {
        *parser = save;
        return false;
    }
    return true;
}

static vm_block_t *vm_paka_blocks_new(vm_paka_blocks_t *blocks) {
    if (blocks->alloc <= blocks->len + 1) {
        blocks->alloc = (blocks->len + 1) * 2;
        blocks->blocks =
            vm_realloc(blocks->blocks, sizeof(vm_block_t *) * blocks->alloc);
    }
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t){
        .id = (ptrdiff_t) blocks->len,
    };
    blocks->blocks[blocks->len++] = block;
    return block;
}

static uint8_t vm_paka_find_reg(size_t *regs) {
    for (size_t i = 0; i < 256; i++) {
        if (regs[i] == 0) {
            regs[i] = 1;
            return (uint8_t)i;
        }
    }
    __builtin_trap();
}

bool vm_paka_parser_branch(vm_paka_parser_t *parser, vm_paka_comp_t *comp,
                           vm_block_t *iftrue, vm_block_t *iffalse) {
    vm_arg_t lhs = vm_paka_parser_expr_base(parser, comp);
    if (lhs.type == VM_ARG_UNK) {
        return false;
    }
    vm_paka_parser_strip_spaces(parser);
    if (vm_paka_parser_match(parser, "==")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BEQ,
            .args[0] = lhs,
            .args[1] = rhs,
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        };
        return true;
    }
    if (vm_paka_parser_match(parser, "~=")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BEQ,
            .args[0] = lhs,
            .args[1] = rhs,
            .targets[0] = iffalse,
            .targets[1] = iftrue,
        };
        return true;
    }
    if (vm_paka_parser_match(parser, "<=")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BLT,
            .args[0] = rhs,
            .args[1] = lhs,
            .targets[0] = iffalse,
            .targets[1] = iftrue,
        };
        return true;
    }
    if (vm_paka_parser_match(parser, ">=")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BLT,
            .args[0] = lhs,
            .args[1] = rhs,
            .targets[0] = iffalse,
            .targets[1] = iftrue,
        };
        return true;
    }
    if (vm_paka_parser_match(parser, "<")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BLT,
            .args[0] = lhs,
            .args[1] = rhs,
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        };
        return true;
    }
    if (vm_paka_parser_match(parser, ">")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_BLT,
            .args[0] = rhs,
            .args[1] = lhs,
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        };
        return true;
    }
    return false;
}

vm_arg_t vm_paka_parser_expr_mul(vm_paka_parser_t *parser,
                                 vm_paka_comp_t *comp) {
    vm_arg_t arg = vm_paka_parser_expr_single(parser, comp);
    if (arg.type == VM_ARG_UNK) {
        return arg;
    }
    while (true) {
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match(parser, "*")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_mul(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_MUL,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = vm_paka_find_reg(comp->regs),
                    },
                .args[0] = arg,
                .args[1] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else if (vm_paka_parser_match(parser, "/")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_mul(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_DIV,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = vm_paka_find_reg(comp->regs),
                    },
                .args[0] = arg,
                .args[1] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else if (vm_paka_parser_match(parser, "%")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_mul(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_MOD,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = vm_paka_find_reg(comp->regs),
                    },
                .args[0] = arg,
                .args[1] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else {
            break;
        }
    }
    return arg;
}

vm_arg_t vm_paka_parser_expr_add(vm_paka_parser_t *parser,
                                 vm_paka_comp_t *comp) {
    vm_arg_t arg = vm_paka_parser_expr_mul(parser, comp);
    if (arg.type == VM_ARG_UNK) {
        return arg;
    }
    while (true) {
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match(parser, "+")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_mul(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_ADD,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = vm_paka_find_reg(comp->regs),
                    },
                .args[0] = arg,
                .args[1] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else if (vm_paka_parser_match(parser, "-")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_mul(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_SUB,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = vm_paka_find_reg(comp->regs),
                    },
                .args[0] = arg,
                .args[1] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else {
            break;
        }
    }
    return arg;
}

vm_arg_t vm_paka_parser_expr_base(vm_paka_parser_t *parser,
                                  vm_paka_comp_t *comp) {
    vm_arg_t arg = vm_paka_parser_expr_add(parser, comp);
    if (arg.type == VM_ARG_UNK) {
        return arg;
    }
    return arg;
}

static char *vm_paka_parser_name(vm_paka_parser_t *parser) {
    size_t len = vm_paka_parser_ident_len(parser);
    char *buf = vm_malloc(sizeof(char) * (len + 1));
    for (size_t i = 0; i < len; i++) {
        buf[i] = vm_paka_parser_read(parser);
    }
    buf[len] = '\0';
    return buf;
}

vm_arg_t vm_paka_parser_postfix(vm_paka_parser_t *parser, vm_paka_comp_t *comp,
                                vm_arg_t arg) {
redo:;
    if (vm_paka_parser_match(parser, "(")) {
        vm_block_t *next = vm_paka_blocks_new(comp->blocks);
        vm_arg_t out = (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        vm_branch_t branch = (vm_branch_t){
            .op = VM_BOP_CALL,
            .out = out,
            .args[0] = arg,
            .targets[0] = next,
        };
        size_t index = 1;
        do {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t xarg = (vm_arg_t){
                .type = VM_ARG_REG,
                .reg = vm_paka_find_reg(comp->regs),
            };
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_MOVE,
                .out = xarg,
                .args[0] = vm_paka_parser_expr_base(parser, comp)};
            vm_block_realloc(comp->write, instr);
            branch.args[index] = xarg;
            index += 1;
            vm_paka_parser_strip_spaces(parser);
        } while (vm_paka_parser_match(parser, ","));
        comp->write->branch = branch;
        comp->write = next;
        if (!vm_paka_parser_match(parser, ")")) {
            return (vm_arg_t){
                .type = VM_ARG_UNK,
            };
        }
        arg = out;
        goto redo;
    }
    if (vm_paka_parser_match(parser, "::")) {
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match_keyword(parser, "type")) {
            vm_arg_t out = (vm_arg_t){
                .type = VM_ARG_REG,
                .reg = vm_paka_find_reg(comp->regs),
            };
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_TYPE,
                .out = out,
                .args[0] = arg,
            };
            vm_block_realloc(comp->write, instr);
            arg = out;
            goto redo;
        }
        return (vm_arg_t){
            .type = VM_ARG_UNK,
        };
    }
    if (vm_paka_parser_match(parser, ".")) {
        vm_paka_parser_strip_spaces(parser);
        vm_block_t *next = vm_paka_blocks_new(comp->blocks);
        vm_arg_t xout = (vm_arg_t) {
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        size_t len = vm_paka_parser_ident_len(parser);
        if (len == 0) {
            return (vm_arg_t){
                .type = VM_ARG_UNK,
            };
        }
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match(parser, "=")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t key = (vm_arg_t){
                .type = VM_ARG_STR,
                .str = buf,
            };
            if (key.type != VM_ARG_REG) {
                vm_arg_t yout = (vm_arg_t){
                    .type = VM_ARG_REG,
                    .reg = vm_paka_find_reg(comp->regs),
                };
                vm_instr_t move = (vm_instr_t){
                    .op = VM_IOP_MOVE,
                    .out = yout,
                    .args[0] = key,
                };
                vm_block_realloc(
                    comp->write,
                    move);
                key = yout;
            }
            vm_arg_t val = vm_paka_parser_expr_base(parser, comp);
            if (val.type != VM_ARG_REG) {
                vm_arg_t yout = (vm_arg_t){
                    .type = VM_ARG_REG,
                    .reg = vm_paka_find_reg(comp->regs),
                };
                vm_instr_t move = (vm_instr_t){
                    .op = VM_IOP_MOVE,
                    .out = yout,
                    .args[0] = val,
                };
                vm_block_realloc(
                    comp->write,
                    move);
                val = yout;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_SET,
                .args[0] = arg,
                .args[1] = key,
                .args[2] = val,
            };
            vm_block_realloc(
                comp->write,
                instr);
            arg = val;
        } else {
            vm_block_realloc(comp->write, (vm_instr_t){
                                              .op = VM_IOP_MOVE,
                                              .out = xout,
                                              .args[0] = (vm_arg_t){
                                                  .type = VM_ARG_STR,
                                                  .str = buf,
                                              },
                                          });
            comp->write->branch = (vm_branch_t){
                .op = VM_BOP_GET,
                .out = xout, 
                .args[0] = arg,
                .args[1] = xout,
                .targets[0] = next,
            };
            comp->write = next;
            arg = xout;
            goto redo;
        }
    }
    if (vm_paka_parser_match(parser, "[")) {
        vm_block_t *next = vm_paka_blocks_new(comp->blocks);
        vm_arg_t out = (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_GET,
            .out = out,
            .args[0] = arg,
            .args[1] = vm_paka_parser_expr_base(parser, comp),
            .targets[0] = next,
        };
        comp->write = next;
        if (!vm_paka_parser_match(parser, "]")) {
            return (vm_arg_t){
                .type = VM_ARG_UNK,
            };
        }
        arg = out;
        goto redo;
    }
    return arg;
}

vm_arg_t vm_paka_parser_expr_single(vm_paka_parser_t *parser,
                                    vm_paka_comp_t *comp) {
    if (vm_paka_parser_match(parser, "\"")) {
        size_t alloc = 16;
        char *str = vm_malloc(sizeof(char) * alloc);
        size_t len = 0;
        while (!vm_paka_parser_match(parser, "\"")) {
            if (len + 2 >= alloc) {
                alloc = (len + 2) * 2;
                str = vm_realloc(str, sizeof(char) * alloc);
            }
            if (vm_paka_parser_match(parser, "\\")) {
                goto err;
            } else {
                str[len++] = vm_paka_parser_read(parser);
            }
        }
        str[len] = '\0';
        vm_arg_t ret = (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        vm_instr_t instr = (vm_instr_t){
            .op = VM_IOP_MOVE,
            .out = ret,
            .args[0] =
                (vm_arg_t){
                    .type = VM_ARG_STR,
                    .str = str,
                },
        };
        vm_block_realloc(comp->write, instr);
        return ret;
    }
    if (vm_paka_parser_match(parser, "#")) {
        vm_arg_t arg = vm_paka_parser_expr_single(parser, comp);
        if (arg.type == VM_ARG_UNK) {
            goto err;
        }
        vm_arg_t out = (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        vm_block_realloc(comp->write, (vm_instr_t){
                                          .op = VM_IOP_LEN,
                                          .out = out,
                                          .args[0] = arg,
                                      });
        return out;
    }
    if ('0' <= vm_paka_parser_peek(parser) &&
        vm_paka_parser_peek(parser) <= '9') {
        double n = 0;
        while (true) {
            char cur = vm_paka_parser_peek(parser);
            if ('0' <= cur && cur <= '9') {
                vm_paka_parser_skip(parser);
                n *= 10;
                n += cur - '0';
                continue;
            }
            return (vm_arg_t){
                .type = VM_ARG_NUM,
                .num.f64 = n,
            };
        }
    }
    if (vm_paka_parser_match(parser, "{}")) {
        vm_arg_t arg = (vm_arg_t) {
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp->regs),
        };
        vm_instr_t new_instr = (vm_instr_t){
            .op = VM_IOP_NEW,
            .out = arg,
        };
        vm_block_realloc(comp->write, new_instr);
        return arg;
    }
    if (vm_paka_parser_match_keyword(parser, "return")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
        if (arg.type == VM_ARG_UNK) {
            goto err;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_RET,
            .args[0] = arg,
        };
        comp->write = vm_paka_blocks_new(comp->blocks);
    }
    if (vm_paka_parser_match_keyword(parser, "def")) {
        vm_paka_parser_strip_spaces(parser);
        char *buf = vm_paka_parser_name(parser);
        if (!vm_paka_parser_match(parser, "(")) {
            goto err;
        }
        vm_paka_name_map_t *last_names = comp->names;
        vm_block_t *last_write = comp->write;
        size_t *last_regs = comp->regs;
        vm_paka_name_map_t names = (vm_paka_name_map_t){0};
        vm_block_t *body = vm_paka_blocks_new(comp->blocks);
        comp->regs = vm_malloc(sizeof(size_t) * 256);
        for (size_t i = 0; i < 256; i++) {
            comp->regs[i] = 0;
        }
        if (comp->nfuncs + 1 >= comp->funcs_alloc) {
            comp->funcs_alloc = (comp->nfuncs + 1) * 2;
            comp->func_names = vm_realloc(comp->func_names,
                                            sizeof(const char *) * comp->funcs_alloc);
            comp->func_blocks = vm_realloc(
                comp->func_blocks, sizeof(vm_block_t *) * comp->funcs_alloc);
        }
        comp->func_names[comp->nfuncs] = buf;
        comp->func_blocks[comp->nfuncs] = body;
        comp->nfuncs += 1;
        comp->names = &names;
        comp->write = body;
        uint8_t reg = 0;
        do {
            reg += 1;
            vm_paka_parser_strip_spaces(parser);
            char *arg = vm_paka_parser_name(parser);
            size_t index = comp->names->len++;
            if (comp->names->len + 1 >= comp->names->alloc) {
                comp->names->alloc = comp->names->len * 2 + 1;
                comp->names->keys = vm_realloc(
                    comp->names->keys, sizeof(const char *) * comp->names->alloc);
                comp->names->values = vm_realloc(
                    comp->names->values, sizeof(uint8_t) * comp->names->alloc);
            }
            comp->regs[reg] = 1;
            comp->names->keys[index] = arg;
            comp->names->values[index] = reg;
            vm_paka_parser_strip_spaces(parser);
        } while (vm_paka_parser_match(parser, ","));
        if (!vm_paka_parser_match(parser, ")")) {
            goto err;
        }
        vm_paka_parser_strip_spaces(parser);
        if (!vm_paka_parser_match(parser, "{")) {
            goto err;
        }
        int xres = vm_paka_parser_block(parser, comp);
        if (xres != VM_PAKA_END) {
            goto err;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_RET,
            .args[0] =
                (vm_arg_t){
                    .type = VM_ARG_NIL,
                    .num.f64 = 0,
                },
        };
        comp->names = last_names;
        comp->write = last_write;
        comp->regs = last_regs;
        return (vm_arg_t){.type = VM_ARG_NIL};
    }
    if (vm_paka_parser_match_keyword(parser, "let")) {
        vm_paka_parser_strip_spaces(parser);
        char *buf = vm_paka_parser_name(parser);
        vm_paka_parser_strip_spaces(parser);
        size_t index = comp->names->len++;
        if (comp->names->len + 1 >= comp->names->alloc) {
            comp->names->alloc = comp->names->len * 2 + 1;
            comp->names->keys = vm_realloc(comp->names->keys,
                                           sizeof(const char *) * comp->names->alloc);
            comp->names->values =
                vm_realloc(comp->names->values, sizeof(uint8_t) * comp->names->alloc);
        }
        comp->names->keys[index] = buf;
        uint8_t reg = vm_paka_find_reg(comp->regs);
        comp->names->values[index] = reg;
        if (vm_paka_parser_match(parser, "=")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
            if (arg.type == VM_ARG_UNK) {
                goto err;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_MOVE,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = reg,
                    },
                .args[0] = arg,
            };
            vm_block_realloc(comp->write, instr);
        }
        return (vm_arg_t){
            .type = VM_ARG_REG,
            .reg = reg,
        };
    }
    if (vm_paka_parser_match(parser, "{")) {
        vm_block_t *body = vm_paka_blocks_new(comp->blocks);
        vm_block_t *after = vm_paka_blocks_new(comp->blocks);
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_JUMP,
            .targets[0] = body,
        };
        comp->write = body;
        int xres = vm_paka_parser_block(parser, comp);
        if (xres != VM_PAKA_END) {
            goto err;
        }
        comp->write->branch = (vm_branch_t){
            .op = VM_BOP_JUMP,
            .targets[0] = after,
        };
        comp->write = after;
        return (vm_arg_t){
            .type = VM_ARG_NONE,
        };
    }
    if (vm_paka_parser_match_keyword(parser, "if")) {
        vm_block_t *body = vm_paka_blocks_new(comp->blocks);
        vm_block_t *after = vm_paka_blocks_new(comp->blocks);
        vm_paka_parser_strip_spaces(parser);
        if (!vm_paka_parser_branch(parser, comp, body, after)) {
            goto err;
        }
        vm_paka_parser_strip_spaces(parser);
        if (!vm_paka_parser_match(parser, "{")) {
            goto err;
        }
        comp->write = body;
        vm_paka_parser_block_full_t xres = vm_paka_parser_block_full(parser, comp);
        if (xres.ret != VM_PAKA_END) {
            goto err;
        }
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match_keyword(parser, "else")) {
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, "{")) {
                goto err;
            }
            vm_block_t *els = after;
            after = vm_paka_blocks_new(comp->blocks);
            comp->write->branch = (vm_branch_t){
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
            comp->write = els;
            vm_paka_parser_block_full_t yres = vm_paka_parser_block_full(parser, comp);
            if (yres.ret != VM_PAKA_END) {
                goto err;
            }
            comp->write->branch = (vm_branch_t){
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
        } else {
            comp->write->branch = (vm_branch_t){
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
        }
        comp->write = after;
        return (vm_arg_t){
            .type = VM_ARG_NONE,
        };
    }
    if (vm_paka_parser_is_ident0_char(vm_paka_parser_peek(parser))) {
        vm_paka_parser_t save = *parser;
        size_t len = vm_paka_parser_ident_len(parser);
        if (len == 0) {
            goto err;
        }
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        if (!strcmp(buf, "env")) {
            vm_arg_t reg = (vm_arg_t){
                .type = VM_ARG_REG,
                .reg = vm_paka_find_reg(comp->regs),
            };
            vm_block_realloc(comp->write, (vm_instr_t){
                                              .op = VM_IOP_STD,
                                              .out = reg,
                                          });
            return vm_paka_parser_postfix(parser, comp, reg);
        }
        if (!strcmp(buf, "rawget")) {
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, "(")) {
                goto err;
            }
            vm_arg_t tab = vm_paka_parser_expr_base(parser, comp);
            if (tab.type == VM_ARG_UNK) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, ",")) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t key = vm_paka_parser_expr_base(parser, comp);
            if (key.type == VM_ARG_UNK) {
                goto err;
            }
            if (!vm_paka_parser_match(parser, ")")) {
                return (vm_arg_t){
                    .type = VM_ARG_UNK,
                };
            }
            vm_block_t *next = vm_paka_blocks_new(comp->blocks);
            uint8_t out = vm_paka_find_reg(comp->regs);
            comp->write->branch = (vm_branch_t){
                .op = VM_BOP_GET,
                .out =
                    (vm_arg_t){
                        .type = VM_ARG_REG,
                        .reg = out,
                    },
                .args[0] = tab,
                .args[1] = key,
                .targets[0] = next,
            };
            comp->write = next;
            return (vm_arg_t){
                .type = VM_ARG_REG,
                .reg = out,
            };
        }
        if (!strcmp(buf, "rawset")) {
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, "(")) {
                goto err;
            }
            vm_arg_t tab = vm_paka_parser_expr_base(parser, comp);
            if (tab.type == VM_ARG_UNK) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, ",")) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t key = vm_paka_parser_expr_base(parser, comp);
            if (key.type == VM_ARG_UNK) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, ",")) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t val = vm_paka_parser_expr_base(parser, comp);
            if (val.type == VM_ARG_UNK) {
                goto err;
            }
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, ")")) {
                goto err;
            }
            vm_instr_t instr = (vm_instr_t){
                .op = VM_IOP_SET,
                .args[0] = tab,
                .args[1] = key,
                .args[2] = val,
            };
            vm_block_realloc(comp->write, instr);
            return (vm_arg_t){
                .type = VM_ARG_NIL,
            };
        }
        for (vm_paka_name_map_t *names = comp->names; names; names = names->next) {
            for (size_t i = 0; i < names->len; i++) {
                if (!strcmp(buf, names->keys[i])) {
                    return vm_paka_parser_postfix(parser, comp,
                                                  (vm_arg_t){
                                                      .type = VM_ARG_REG,
                                                      .reg = names->values[i],
                                                  });
                }
            }
        }
        for (size_t i = 0; i < comp->nfuncs; i++) {
            if (!strcmp(buf, comp->func_names[i])) {
                vm_paka_parser_strip_spaces(parser);
                vm_block_t *block = comp->func_blocks[i];
                vm_branch_t branch = (vm_branch_t){
                    .op = VM_BOP_CALL,
                    .args[0] =
                        (vm_arg_t){
                            .type = VM_ARG_FUNC,
                            .func = block,
                        },
                };
                size_t head = 1;
                if (!vm_paka_parser_match(parser, "(")) {
                    goto err;
                }
                if (!vm_paka_parser_match(parser, ")")) {
                    do {
                        vm_paka_parser_strip_spaces(parser);
                        vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
                        if (arg.type == VM_ARG_UNK) {
                            goto err;
                        }
                        if (arg.type == VM_ARG_REG) {
                            branch.args[head++] = arg;
                        } else {
                            vm_arg_t real = (vm_arg_t){
                                .type = VM_ARG_REG,
                                .reg = vm_paka_find_reg(comp->regs),
                            };
                            vm_block_realloc(comp->write, (vm_instr_t){
                                                              .op = VM_IOP_MOVE,
                                                              .out = real,
                                                              .args[0] = arg,
                                                          });
                            branch.args[head++] = real;
                        }
                        vm_paka_parser_strip_spaces(parser);
                    } while (vm_paka_parser_match(parser, ","));
                    if (!vm_paka_parser_match(parser, ")")) {
                        goto err;
                    }
                }
                branch.out = (vm_arg_t){
                    .type = VM_ARG_REG,
                    .reg = vm_paka_find_reg(comp->regs),
                };
                vm_block_t *then = vm_paka_blocks_new(comp->blocks);
                branch.targets[0] = then;
                comp->write->branch = branch;
                comp->write = then;
                return vm_paka_parser_postfix(parser, comp, branch.out);
            }
        }
        fprintf(stderr, "unknown name: %s (at %zu:%zu)\n", buf, parser->line,
                parser->col);
        goto err;
    }
err:;
    return (vm_arg_t){
        .type = VM_ARG_UNK,
    };
}

vm_paka_parser_block_full_t vm_paka_parser_block_full(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    vm_paka_name_map_t names = (vm_paka_name_map_t){
        .next = comp->names,
    };
    comp->names = &names;
    vm_arg_t arg = (vm_arg_t){
        .type = VM_ARG_NIL,
    };
    int ret = -1;
    vm_paka_parser_strip_spaces(parser);
    while (vm_paka_parser_peek(parser) != '\0') {
        vm_paka_parser_strip_spaces(parser);
        arg = vm_paka_parser_expr_base(parser, comp);
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match(parser, "}")) {
            ret = VM_PAKA_END;
            break;
        }
        if (arg.type == VM_ARG_UNK) {
            break;
        }
    }
    vm_paka_parser_strip_spaces(parser);
    comp->names = names.next;
    return (vm_paka_parser_block_full_t) {
        .ret = ret,
        .arg = arg,
    };
}

int vm_paka_parser_block(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    return vm_paka_parser_block_full(parser, comp).ret;
}

typedef struct {
    vm_paka_blocks_t blocks;
    vm_block_t *block;
}  vm_paka_parse_result_t;

static vm_paka_parse_result_t vm_paka_parse_internal(const char *src) {
    vm_paka_parser_t parser = (vm_paka_parser_t){
        .src = src,
        .index = 0,
        .line = 1,
        .col = 1,
    };
    vm_paka_blocks_t blocks = (vm_paka_blocks_t){0};
    vm_block_t *block = vm_paka_blocks_new(&blocks);
    size_t *regs = vm_malloc(sizeof(size_t) * 256);
    for (size_t i = 0; i < 256; i++) {
        regs[i] = 0;
    }
    vm_paka_comp_t comp = (vm_paka_comp_t){
        .write = block,
        .regs = regs,
        .blocks = &blocks,
    };
    vm_paka_parser_block_full_t full = vm_paka_parser_block_full(&parser, &comp);
    if (vm_paka_parser_peek(&parser) != '\0') {
        fprintf(stderr, "error(2) at line %zu, col %zu\n", parser.line, parser.col);
        return (vm_paka_parse_result_t) {0};
    }
    // vm_print_arg(stdout, full.arg);
    // printf("\n\n");
    if (full.arg.type != VM_ARG_NONE && full.arg.type != VM_ARG_UNK && full.arg.type != VM_ARG_REG) {
        vm_arg_t arg = (vm_arg_t) {
            .type = VM_ARG_REG,
            .reg = vm_paka_find_reg(comp.regs),
        };
        vm_block_realloc(comp.write, (vm_instr_t) {
            .op = VM_IOP_MOVE,
            .out = arg,
            .args[0] = full.arg,
        });
        full.arg = arg;
    }
    comp.write->branch = (vm_branch_t){
        .op = VM_BOP_RET,
        .args[0] = full.arg,
    };
    vm_block_info(blocks.len, blocks.blocks);
#if defined(VM_DUMP_PARSE)
    for (size_t i = 0; i < blocks.len; i++) {
        vm_print_block(stdout, blocks.blocks[i]);
    }
#endif
    return (vm_paka_parse_result_t) {
        .block = block,
        .blocks = blocks,
    };
}

vm_block_t *vm_paka_parse(const char *src) {
    return vm_paka_parse_internal(src).block;
}

vm_paka_blocks_t vm_paka_parse_blocks(const char *src) {
    return vm_paka_parse_internal(src).blocks;
}
