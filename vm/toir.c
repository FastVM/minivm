
#include "toir.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

#include "const.h"
#include "tag.h"

struct vm_parser_t;
typedef struct vm_parser_t vm_parser_t;

struct vm_parser_t {
    size_t len;
    size_t alloc;
    const char **names;
    vm_block_t **blocks;
    const char **src;
    size_t line;
    size_t col;
};

static char *vm_strdup(const char *str) {
    size_t len = strlen(str);
    char *ret = vm_malloc(sizeof(char) * (len + 1));
    for (size_t i = 0; i <= len; i++) {
        ret[i] = str[i];
    }
    return ret;
}

static void vm_skip(vm_parser_t *state) {
    char got = **state->src;
    if (got == '\n') {
        state->line += 1;
        state->col = 1;
    } else {
        state->col += 1;
    }
    *state->src += 1;
}

static vm_block_t *vm_parse_find(vm_parser_t *state, const char *name) {
    for (size_t i = 0; i < state->len; i++) {
        if (!strcmp(state->names[i], name)) {
            return state->blocks[i];
        }
    }
    size_t where = state->len++;
    if (where >= state->alloc) {
        state->alloc = state->len * 4;
        state->names = vm_realloc(state->names, sizeof(const char *) * state->alloc);
        state->blocks = vm_realloc(state->blocks, sizeof(vm_block_t *) * state->alloc);
    }
    state->names[where] = vm_strdup(name);
    vm_block_t *block = vm_malloc(sizeof(vm_block_t));
    *block = (vm_block_t){
        .tag = VM_TAG_FN,
    };
    state->blocks[where] = block;
    return state->blocks[where];
}

static void vm_parse_strip(vm_parser_t *state) {
    while (**state->src != '\0' && isspace(**state->src)) {
        vm_skip(state);
    }
}

static const char *vm_parse_word_until(vm_parser_t *state, char stop) {
    vm_parse_strip(state);
    size_t maxlen = 8;
    char *name = vm_malloc(sizeof(char) * maxlen);
    size_t len = 0;
    while (**state->src != '\0' && **state->src != stop && !isspace(**state->src)) {
        name[len] = **state->src;
        len += 1;
        vm_skip(state);
        if (len >= maxlen) {
            maxlen = len * 2;
            name = vm_realloc(name, sizeof(char) * maxlen);
        }
        name[len] = '\0';
    }
    return name;
}

static uint8_t vm_parse_tag(vm_parser_t *state) {
    uint8_t tag = VM_TAG_UNK;
    const char *tname = vm_parse_word_until(state, '\0');
    if (!strcmp(tname, "b")) {
        tag = VM_TAG_I64;
    }
    if (!strcmp(tname, "i64")) {
        tag = VM_TAG_I64;
    }
    if (!strcmp(tname, "f64")) {
        tag = VM_TAG_F64;
    }
    vm_free(tname);
    return tag;
}

static vm_block_t *vm_parse_arg_block(vm_parser_t *state) {
    vm_parse_strip(state);
    if (**state->src != '[') {
        fprintf(stderr, "expecting block name argument's opening `[`\n");
        return NULL;
    }
    vm_skip(state);
    const char *name = vm_parse_word_until(state, ']');
    vm_block_t *block = vm_parse_find(state, name);
    vm_free(name);
    vm_parse_strip(state);
    vm_skip(state);
    return block;
}

static vm_arg_t vm_parse_arg(vm_parser_t *state) {
    vm_parse_strip(state);
    vm_arg_t arg = (vm_arg_t) {.type = VM_ARG_UNK};
    if (**state->src == '%') {
        vm_skip(state);
        size_t n = 0;
        while (isdigit(**state->src)) {
            n *= 10;
            n += **state->src - '0';
            vm_skip(state);
        }
        arg.reg = n;
        arg.type = VM_ARG_REG;
    }
    if (isdigit(**state->src)) {
        double n = 0;
        while (isdigit(**state->src)) {
            n *= 10;
            n += **state->src - '0';
            vm_skip(state);
        }
        if (**state->src == '.') {
            vm_skip(state);
            double div = 1;
            while (isdigit(**state->src)) {
                div *= 10;
                n *= 10;
                n += **state->src - '0';
                vm_skip(state);
            }
            n /= div;
        }
        arg.num = n;
        arg.type = VM_ARG_NUM;
    }
    if (**state->src == '[') {
        arg.func = vm_parse_arg_block(state);
        arg.type = VM_ARG_FUNC;
    }
    return arg;
}

static bool vm_parse_state(vm_parser_t *state) {
    vm_block_t *block = NULL;
    while (**state->src != '\0') {
        vm_parse_strip(state);
        if (**state->src == '@') {
            vm_skip(state);
            const char *name = vm_parse_word_until(state, ':');
            vm_skip(state);
            block = vm_parse_find(state, name);
            vm_free(name);
        } else {
            const char *name = vm_parse_word_until(state, '.');
            if (block == NULL) {
                fprintf(stderr, "%s not within a block\n", name);
                vm_free(name);
                goto fail;
            }
            if (!strcmp(name, "blt") || !strcmp(name, "beq") || !strcmp(name, "bb") || !strcmp(name, "ret") || !strcmp(name, "jump") || !strcmp(name, "exit")) {
                vm_branch_t branch = (vm_branch_t) {
                    .op = VM_BOP_FALL,
                    .tag = VM_TAG_UNK,
                };
                if (!strcmp(name, "bb")) {
                    branch.op = VM_BOP_BOOL;
                }
                if (!strcmp(name, "blt")) {
                    branch.op = VM_BOP_LESS;
                }
                if (!strcmp(name, "beq")) {
                    branch.op = VM_BOP_EQUAL;
                }
                if (!strcmp(name, "jump")) {
                    branch.op = VM_BOP_JUMP;
                }
                if (!strcmp(name, "ret")) {
                    branch.op = VM_BOP_RET;
                }
                if (!strcmp(name, "exit")) {
                    branch.op = VM_BOP_EXIT;
                }
                vm_free(name);
                if (**state->src == '.') {
                    vm_skip(state);
                    branch.tag = vm_parse_tag(state);
                }
                switch (branch.op) {
                case VM_BOP_JUMP: {
                    branch.targets[0] = vm_parse_arg_block(state);
                    if (branch.targets[0] == NULL) {
                        goto fail;
                    }
                    break;
                }
                case VM_BOP_BOOL: {
                    branch.args[0] = vm_parse_arg(state);
                    branch.targets[0] = vm_parse_arg_block(state);
                    if (branch.targets[0] == NULL) {
                        goto fail;
                    }
                    branch.targets[1] = vm_parse_arg_block(state);
                    if (branch.targets[0] == NULL) {
                        goto fail;
                    }
                    break;
                }
                case VM_BOP_LESS: {
                    branch.args[0] = vm_parse_arg(state);
                    branch.args[1] = vm_parse_arg(state);
                    branch.targets[0] = vm_parse_arg_block(state);
                    if (branch.targets[0] == NULL) {
                        goto fail;
                    }
                    branch.targets[1] = vm_parse_arg_block(state);
                    if (branch.targets[1] == NULL) {
                        goto fail;
                    }
                    break;
                }
                case VM_BOP_EQUAL: {
                    branch.args[0] = vm_parse_arg(state);
                    branch.args[1] = vm_parse_arg(state);
                    branch.targets[0] = vm_parse_arg_block(state);
                    if (branch.targets[0] == NULL) {
                        goto fail;
                    }
                    branch.targets[1] = vm_parse_arg_block(state);
                    if (branch.targets[1] == NULL) {
                        goto fail;
                    }
                    break;
                }
                case VM_BOP_RET: {
                    branch.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_BOP_EXIT: {
                    break;
                }
                }
                block->branch = branch;
                block = NULL;
            } else {
                vm_instr_t instr = (vm_instr_t) {
                    .op = VM_IOP_NOP,
                    .tag = VM_TAG_UNK,
                };
                if (!strcmp(name, "add")) {
                    instr.op = VM_IOP_ADD;
                }
                if (!strcmp(name, "sub")) {
                    instr.op = VM_IOP_SUB;
                }
                if (!strcmp(name, "mul")) {
                    instr.op = VM_IOP_MUL;
                }
                if (!strcmp(name, "div")) {
                    instr.op = VM_IOP_DIV;
                }
                if (!strcmp(name, "mod")) {
                    instr.op = VM_IOP_MOD;
                }
                if (!strcmp(name, "call")) {
                    instr.op = VM_IOP_CALL;
                }
                if (!strcmp(name, "arr")) {
                    instr.op = VM_IOP_ARR;
                }
                if (!strcmp(name, "tab")) {
                    instr.op = VM_IOP_TAB;
                }
                if (!strcmp(name, "get")) {
                    instr.op = VM_IOP_GET;
                }
                if (!strcmp(name, "set")) {
                    instr.op = VM_IOP_SET;
                }
                if (!strcmp(name, "len")) {
                    instr.op = VM_IOP_LEN;
                }
                if (!strcmp(name, "type")) {
                    instr.op = VM_IOP_TYPE;
                }
                if (!strcmp(name, "out")) {
                    instr.op = VM_IOP_OUT;
                }
                if (!strcmp(name, "in")) {
                    instr.op = VM_IOP_IN;
                }
                if (!strcmp(name, "bnot")) {
                    instr.op = VM_IOP_BNOT;
                }
                if (!strcmp(name, "bor")) {
                    instr.op = VM_IOP_BOR;
                }
                if (!strcmp(name, "bxor")) {
                    instr.op = VM_IOP_BXOR;
                }
                if (!strcmp(name, "bshl")) {
                    instr.op = VM_IOP_BSHL;
                }
                if (!strcmp(name, "bshr")) {
                    instr.op = VM_IOP_BSHR;
                }
                if (instr.op == VM_IOP_NOP) {
                    fprintf(stderr, "unknown name: %s\n", name);
                    vm_free(name);
                    return true;
                }
                vm_free(name);
                if (**state->src == '.') {
                    vm_skip(state);
                    instr.tag = vm_parse_tag(state);
                }
                switch (instr.op) {
                case VM_IOP_MOVE: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_ADD: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_SUB: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_MUL: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_DIV: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_MOD: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_CALL: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    for (size_t i = 1; **state->src != '\n'; i++) {
                        instr.args[i] = vm_parse_arg(state);
                        vm_parse_strip(state);
                    }
                    break;
                }
                case VM_IOP_ARR: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_TAB: {
                    instr.out = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_GET: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_SET: {
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    instr.args[2] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_LEN: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_TYPE: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_OUT: {
                    instr.args[0] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_IN: {
                    instr.out = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_BOR: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_BAND: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_BXOR: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_BSHL: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                case VM_IOP_BSHR: {
                    instr.out = vm_parse_arg(state);
                    instr.args[0] = vm_parse_arg(state);
                    instr.args[1] = vm_parse_arg(state);
                    break;
                }
                }
                vm_block_realloc(block, instr);
            }
        }
        vm_parse_strip(state);
    }
    return false;
fail:
    fprintf(stderr, "error on line %zu, col %zu", state->line, state->col);
    return true;
}

vm_block_t *vm_parse(const char *src) {
    vm_parser_t parse;
    parse.names = NULL;
    parse.blocks = NULL;
    parse.len = 0;
    parse.alloc = 0;
    parse.line = 1;
    parse.col = 1;
    parse.src = &src;
    bool bad = vm_parse_state(&parse);
    if (bad) {
        return NULL;
    }
    return vm_parse_find(&parse, "");
}
