#include "paka.h"

enum {
    VM_PAKA_END,
    VM_PAKA_ELSE,
    VM_PAKA_ELSEIF,
};

bool vm_paka_parser_is_ident0_char(char c) {
    return ('A' <= c && c <= 'Z') ||  ('a' <= c && c <= 'z');
}

bool vm_paka_parser_is_ident1_char(char c) {
    return vm_paka_parser_is_ident0_char(c) || ('0' <= c && c <= '9') || c == ':' || c == '.';
}

bool vm_paka_parser_is_space_char(char c) {
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

size_t vm_paka_parser_tell(vm_paka_parser_t *parser) {
    return parser->index;
}

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

bool vm_paka_parser_match_if(vm_paka_parser_t *parser, bool(*fn)(char)) {
    if (!fn(vm_paka_parser_peek(parser))) {
        return false;
    }
    vm_paka_parser_skip(parser);
    return true;
}

void vm_paka_parser_strip_spaces(vm_paka_parser_t *parser) {
    while (vm_paka_parser_match_if(parser, vm_paka_parser_is_space_char)) {}
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

bool vm_paka_parser_match_keyword(vm_paka_parser_t *parser, const char *keyword) {
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

vm_block_t *vm_paka_blocks_new(vm_paka_blocks_t *blocks) {
    if (blocks->alloc <= blocks->len + 1) {
        blocks->alloc = (blocks->len + 1) * 2;
        blocks->blocks = vm_realloc(blocks->blocks, sizeof(vm_block_t *) * blocks->alloc);
    }
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t) {
        .id = blocks->len,
    };
    blocks->blocks[blocks->len++] = block;
    return block;
}

uint8_t vm_paka_find_reg(size_t *regs) {
    for (size_t i = 0; i < 256; i++) {
        if (regs[i] == 0) {
            regs[i] = 1;
            return (uint8_t) i;
        }
    }
    __builtin_trap();
}

bool vm_paka_parser_branch(vm_paka_parser_t *parser, vm_paka_comp_t *comp, vm_block_t *iftrue, vm_block_t *iffalse) {
    vm_arg_t lhs = vm_paka_parser_expr_base(parser, comp);
    if (lhs.type == VM_ARG_UNK) {
        return false;
    }
    vm_paka_parser_strip_spaces(parser);
    if (vm_paka_parser_match(parser, "<")) {
        vm_paka_parser_strip_spaces(parser);
        vm_arg_t rhs = vm_paka_parser_expr_base(parser, comp);
        if (rhs.type == VM_ARG_UNK) {
            return false;
        }
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_BLT,
            .args[0] = lhs,
            .args[1] = rhs,
            .targets[0] = iftrue,
            .targets[1] = iffalse,
        };
        return true;
    } 
    return false;
}

vm_arg_t vm_paka_parser_expr_mul(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_MUL,
                .out = (vm_arg_t) {
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_DIV,
                .out = (vm_arg_t) {
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_MOD,
                .out = (vm_arg_t) {
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

vm_arg_t vm_paka_parser_expr_add(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_ADD,
                .out = (vm_arg_t) {
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_SUB,
                .out = (vm_arg_t) {
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

vm_arg_t vm_paka_parser_expr_base(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    vm_arg_t arg = vm_paka_parser_expr_add(parser, comp);
    if (arg.type == VM_ARG_UNK) {
        return arg;
    }
    while (true) {
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match(parser, "=")) {
            vm_paka_parser_strip_spaces(parser);
            vm_arg_t arg2 = vm_paka_parser_expr_add(parser, comp);
            if (arg2.type == VM_ARG_UNK) {
                return arg2;
            }
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_MOVE,
                .out = arg,
                .args[0] = arg2,
            };
            vm_block_realloc(comp->write, instr);
            arg = instr.out;
        } else {
            break;
        }
    }
    return arg;
}

vm_arg_t vm_paka_parser_expr_single(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    if (vm_paka_parser_match(parser, "#")) {
        if (vm_paka_parser_match(parser, "\\")) {
            if (vm_paka_parser_match(parser, "tab")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\t',
                };
            } else if (vm_paka_parser_match(parser, "return")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\t',
                };
            } else if (vm_paka_parser_match(parser, "newline")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\n',
                };
            } else if (vm_paka_parser_match(parser, "escape") || vm_paka_parser_match(parser, "esc")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = 27,
                };
            } else {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = (double) vm_paka_parser_read(parser),
                };
            }
        }
        goto err; 
    }
    if ('0' <= vm_paka_parser_peek(parser) && vm_paka_parser_peek(parser) <= '9') {
        size_t n = 0;
        while (true) {
            char cur = vm_paka_parser_peek(parser);
            if ('0' <= cur && cur <= '9') {
                vm_paka_parser_skip(parser);
                n *= 10;
                n += cur - '0';
                continue;
            }
            return (vm_arg_t) {
                .type = VM_ARG_NUM,
                .num = (double) n,
            };
        }
    }
    if (vm_paka_parser_match_keyword(parser, "return")) {
        vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
        if (arg.type == VM_ARG_UNK) {
            goto err;
        }
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_RET,
            .args[0] = arg,
        };
        comp->write = vm_paka_blocks_new(comp->blocks);
    }
    if (vm_paka_parser_match_keyword(parser, "local")) {
        vm_paka_parser_skip(parser);
        vm_paka_parser_strip_spaces(parser);
        size_t len = vm_paka_parser_ident_len(parser);
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        vm_paka_parser_strip_spaces(parser);
        size_t index = comp->names->len++;
        if (comp->names->len + 1 >= comp->names->alloc) {
            comp->names->alloc = comp->names->len * 2 + 1;
            comp->names->keys = vm_realloc(comp->names->keys, sizeof(const char *) * comp->names->alloc);
            comp->names->values = vm_realloc(comp->names->values, sizeof(uint8_t) * comp->names->alloc);
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
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_MOVE,
                .out = (vm_arg_t) {
                    .type = VM_ARG_REG,
                    .reg = reg,
                },
                .args[0] = arg,
            };
            vm_block_realloc(comp->write, instr);
        }
        return (vm_arg_t) { .type = VM_ARG_NONE };
    }
    if (vm_paka_parser_match_keyword(parser, "while")) {
        vm_block_t *check = vm_paka_blocks_new(comp->blocks);
        vm_block_t *body = vm_paka_blocks_new(comp->blocks);
        vm_block_t *after = vm_paka_blocks_new(comp->blocks);
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_JUMP,
            .targets[0] = check,
        };
        vm_paka_parser_strip_spaces(parser);
        comp->write = check;
        if (!vm_paka_parser_branch(parser, comp, body, after)) {
            goto err;
        }
        vm_paka_parser_strip_spaces(parser);
        if (!vm_paka_parser_match_keyword(parser, "do")) {
            goto err;
        }
        comp->write = body;
        vm_paka_parser_block(parser, comp);
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_JUMP,
            .targets[0] = check,
        };
        comp->write = after;
        return (vm_arg_t) {
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
        if (!vm_paka_parser_match_keyword(parser, "then")) {
            goto err;
        }
        comp->write = body;
        int res = vm_paka_parser_block(parser, comp);
        if (res < 0) {
            goto err;
        }
        if (res == VM_PAKA_ELSE) {
            vm_block_t *els = after;
            after = vm_paka_blocks_new(comp->blocks);
            comp->write->branch = (vm_branch_t) {
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
            comp->write = els;
            int res = vm_paka_parser_block(parser, comp);
            if (res != VM_PAKA_END) {
                goto err;
            }
            comp->write->branch = (vm_branch_t) {
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
        } else {
            comp->write->branch = (vm_branch_t) {
                .op = VM_BOP_JUMP,
                .targets[0] = after,
            };
        }
        comp->write = after;
        return (vm_arg_t) {
            .type = VM_ARG_NONE,
        };
    }
    if (vm_paka_parser_is_ident0_char(vm_paka_parser_peek(parser))) {
        size_t len = vm_paka_parser_ident_len(parser);
        if (len == 0) {
            goto err; 
        }
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        if (!strcmp(buf, "print")) {
            vm_paka_parser_strip_spaces(parser);
            if (!vm_paka_parser_match(parser, "(")) {
                goto err;
            }
            bool sep = false;
            do {
                if (sep) {
                    vm_block_realloc(comp->write, (vm_instr_t) {
                        .op = VM_IOP_OUT,
                        .args[0] = (vm_arg_t) {
                            .type = VM_ARG_NUM,
                            .num = (double) '\t',
                        },
                    });
                } else {
                    sep = true;
                }
                vm_paka_parser_strip_spaces(parser);
                vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
                if (arg.type == VM_ARG_UNK) {
                    goto err;
                }
                vm_instr_t instr = (vm_instr_t) {
                    .op = VM_IOP_PRINT,
                    .args[0] = arg,
                };
                vm_block_realloc(comp->write, instr);
                vm_paka_parser_strip_spaces(parser);
            } while (vm_paka_parser_match(parser, ","));
            if (!vm_paka_parser_match(parser, ")")) {
                goto err;
            }
            vm_block_realloc(comp->write, (vm_instr_t) {
                .op = VM_IOP_OUT,
                .args[0] = (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = (double) '\n',
                },
            });
            return (vm_arg_t) {
                .type = VM_ARG_NONE,
            };
        }
        for (vm_paka_name_map_t *names = comp->names; names; names = names->next) {
            for (size_t i = 0; i < names->len; i++) {
                if (!strcmp(buf, names->keys[i])) {
                    return (vm_arg_t) {
                        .type = VM_ARG_REG,
                        .reg = names->values[i],
                    };
                }
            }
        }
        fprintf(stderr, "unknown name: %s (at %zu:%zu)\n", buf, parser->line, parser->col);
        goto err;
    }
err:;
    return (vm_arg_t) {
        .type = VM_ARG_UNK,
    };
}

int vm_paka_parser_block(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    vm_paka_name_map_t names = (vm_paka_name_map_t) {
        .next = comp->names,
    };
    comp->names = &names;
    vm_arg_t arg = (vm_arg_t) {
        .type = VM_ARG_NIL,
    };
    int ret = -1;
    vm_paka_parser_strip_spaces(parser);
    while (vm_paka_parser_peek(parser) != '\0') {
        vm_paka_parser_strip_spaces(parser);
        arg = vm_paka_parser_expr_base(parser, comp);
        if (arg.type == VM_ARG_UNK) {
            break;
        }
        vm_paka_parser_strip_spaces(parser);
        if (vm_paka_parser_match_keyword(parser, "else")) {
            ret = VM_PAKA_ELSE;
            break;
        }
        if (vm_paka_parser_match_keyword(parser, "elseif")) {
            ret = VM_PAKA_ELSEIF;
            break;
        }
        if (vm_paka_parser_match_keyword(parser, "end")) {
            ret = VM_PAKA_END;
            break;
        }
    }
    vm_paka_parser_strip_spaces(parser);
    comp->names = names.next;
    return ret;
}

vm_block_t *vm_paka_parse(const char *src) {
    vm_paka_parser_t parser = (vm_paka_parser_t) {
        .src = src,
        .index = 0,
        .line = 1,
        .col = 1,
    };
    vm_paka_blocks_t blocks = (vm_paka_blocks_t) {0};
    vm_block_t *block = vm_paka_blocks_new(&blocks);
    size_t nregs = 256;
    size_t *regs = vm_malloc(sizeof(size_t) * nregs);
    for (size_t i = 0; i < 256; i++) {
        regs[i] = 0;
    }
    vm_paka_comp_t comp = (vm_paka_comp_t) {
        .write = block,
        .regs = regs,
        .blocks = &blocks,
    };
    vm_paka_parser_block(&parser, &comp);
    if (vm_paka_parser_peek(&parser) != '\0') {
        fprintf(stderr, "error(2) at line %zu, col %zu\n", parser.line, parser.col);
        return NULL;
    }
    comp.write->branch = (vm_branch_t) {
        .op = VM_BOP_EXIT,
    };
    vm_block_info(blocks.len, blocks.blocks);
    // for (size_t i = 0; i < blocks.len; i++) {
    //     vm_print_block(stderr, blocks.blocks[i]);
    // }
    return block;
}