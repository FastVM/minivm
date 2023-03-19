#include "paka.h"

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

vm_arg_t vm_paka_parser_expr_base(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    vm_paka_parser_strip_spaces(parser);
    // if (vm_paka_parser_match_keyword(parser, "out")) {
    //     vm_arg_t arg = vm_paka_parser_expr_base(parser, comp);
    //     if (arg.type == VM_ARG_UNK) {
    //         return arg;
    //     }
    //     vm_instr_t instr = (vm_instr_t) {
    //         .op = VM_IOP_OUT,
    //         .args[0] = arg,
    //         .args[1].type = VM_ARG_NONE,
    //         .out.type = VM_ARG_NONE,
    //     };
    //     vm_block_realloc(comp->write, instr);
    //     return (vm_arg_t) {
    //         .type = VM_ARG_NIL,
    //     };
    // }
    char first = vm_paka_parser_peek(parser);
    if (vm_paka_parser_match(parser, "'")) {
        char value = vm_paka_parser_read(parser);
        if (value == '\\') {
            if (vm_paka_parser_match(parser, "t")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\t',
                };
            } else if (vm_paka_parser_match(parser, "r")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\t',
                };
            } else if (vm_paka_parser_match(parser, "n")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = '\n',
                };
            } else if (vm_paka_parser_match(parser, "e")) {
                return (vm_arg_t) {
                    .type = VM_ARG_NUM,
                    .num = 27,
                };
            } else {
                goto err; 
            }
        }
        if (vm_paka_parser_match(parser, "'")) {
            return (vm_arg_t) {
                .type = VM_ARG_NUM,
                .num = (double) vm_paka_parser_read(parser),
            };
        }
        goto err; 
    }
    if ('0' <= first && first <= '9') {
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
    if (first == '(') {
        vm_paka_parser_skip(parser);
        vm_paka_parser_strip_spaces(parser);
        size_t len = vm_paka_parser_ident_len(parser);
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        vm_paka_parser_strip_spaces(parser);
        if (!strcmp(buf, "asm")) {
            vm_free(buf);
            len = vm_paka_parser_ident_len(parser);
            buf = vm_malloc(sizeof(char) * (len + 1));
            for (size_t i = 0; i < len; i++) {
                buf[i] = vm_paka_parser_read(parser);
            }
            buf[len] = '\0';
            if (!strcmp(buf, "lt")) {
                vm_paka_comp_t save = *comp;
                vm_block_t *ift = vm_malloc(sizeof(vm_block_t));
                vm_block_t *iff = vm_malloc(sizeof(vm_block_t));
                vm_block_t *after = vm_malloc(sizeof(vm_block_t));
                *ift = (vm_block_t) {0};
                *iff = (vm_block_t) {0};
                *after = (vm_block_t) {0};
                comp->write->branch = (vm_branch_t) {
                    .op = VM_BOP_BLT,
                    .args[0] = vm_paka_parser_expr_base(parser, comp),
                    .args[1] = vm_paka_parser_expr_base(parser, comp),
                    .targets[0] = ift,
                    .targets[1] = iff,
                };
                comp->write = ift;
                vm_paka_parser_expr_base(parser, comp);
                ift->branch = (vm_branch_t) {
                    .op = VM_BOP_JUMP,
                    .targets[0] = after,
                };
                comp->write = iff;
                vm_paka_parser_expr_base(parser, comp);
                iff->branch = (vm_branch_t) {
                    .op = VM_BOP_JUMP,
                    .targets[0] = after,
                };
                *comp = save;
                comp->write = after;
            }
            vm_instr_t instr = (vm_instr_t) {
                .op = VM_IOP_NOP,
            };
            if (!strcmp(buf, "move")) {
                instr.op = VM_IOP_MOVE;
            }
            if (!strcmp(buf, "cast")) {
                instr.op = VM_IOP_CAST;
            }
            if (!strcmp(buf, "add")) {
                instr.op = VM_IOP_ADD;
            }
            if (!strcmp(buf, "sub")) {
                instr.op = VM_IOP_SUB;
            }
            if (!strcmp(buf, "mul")) {
                instr.op = VM_IOP_MUL;
            }
            if (!strcmp(buf, "div")) {
                instr.op = VM_IOP_DIV;
            }
            if (!strcmp(buf, "mod")) {
                instr.op = VM_IOP_MOD;
            }
            if (!strcmp(buf, "call")) {
                instr.op = VM_IOP_CALL;
            }
            if (!strcmp(buf, "out")) {
                instr.op = VM_IOP_OUT;
            }
            vm_free(buf);
            size_t i = 0;
            vm_paka_parser_strip_spaces(parser);
            while (!vm_paka_parser_match(parser, ")") && vm_paka_parser_peek(parser) != '\0') {
                instr.args[i++] = vm_paka_parser_expr_base(parser, comp);
                vm_paka_parser_strip_spaces(parser);
            }
            vm_block_realloc(comp->write, instr);
            return instr.out;
        }
        goto err; 
    }
    if (vm_paka_parser_is_ident0_char(first)) {
        size_t len = vm_paka_parser_ident_len(parser);
        if (len == 0) {
            goto err; 
        }
        char *buf = vm_malloc(sizeof(char) * (len + 1));
        for (size_t i = 0; i < len; i++) {
            buf[i] = vm_paka_parser_read(parser);
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }
err:;
    return (vm_arg_t) {
        .type = VM_ARG_UNK,
    };
}

void vm_paka_parser_block(vm_paka_parser_t *parser, vm_paka_comp_t *comp) {
    vm_arg_t arg = (vm_arg_t) {
        .type = VM_ARG_NIL,
    };
    while (!vm_paka_parser_match(parser, "}") && vm_paka_parser_peek(parser) != '\0') {
        arg = vm_paka_parser_expr_base(parser, comp);
        if (arg.type == VM_ARG_UNK) {
            break;
        }
    }
    if (comp->jump != NULL) {
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_JUMP,
            .args[0].type = VM_ARG_NONE,
            .targets[0] = comp->jump,
        };
    } else {
        comp->write->branch = (vm_branch_t) {
            .op = VM_BOP_RET,
            .args[0] = arg,
            .args[1].type = VM_ARG_NONE,
        };
    }
}

vm_block_t *vm_paka_parse(const char *src) {
    vm_paka_parser_t parser = (vm_paka_parser_t) {
        .src = src,
        .index = 0,
        .line = 1,
        .col = 1,
    };
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t) {0};
    vm_block_t *next = vm_malloc(sizeof(vm_block_t));
    *next = (vm_block_t) {0};
    next->branch = (vm_branch_t) {
        .op = VM_BOP_EXIT,
    };
    size_t nregs = 256;
    uint8_t *regs = vm_malloc(sizeof(uint8_t) * nregs);
    for (size_t i = 0; i < 256; i++) {
        regs[i] = 0;
    }
    vm_paka_comp_t comp = (vm_paka_comp_t) {
        .write = block,
        .jump = next,
        .regs = regs,
    };
    vm_paka_parser_block(&parser, &comp);
    if (vm_paka_parser_peek(&parser) != '\0') {
        fprintf(stderr, "error(2) at line %zu, col %zu\n", parser.line, parser.col);
        return NULL;
    }
    return comp.write;
}
