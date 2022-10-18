#include <stdint.h>

#include "./int3.h"

vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->nlocals = nregs;
    ret->locals = vm_malloc(sizeof(vm_value_t) * (ret->nlocals));
    ret->ips = vm_malloc(sizeof(vm_opcode_t *) * (ret->nlocals / VM_NREGS));
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->ips);
    vm_free(state->locals);
    vm_free(state);
}

vm_rblock_t *vm_rblock_new(vm_block_t *block, uint8_t *regs) {
    vm_rblock_t *rblock = vm_malloc(sizeof(vm_rblock_t));
    rblock->block = block;
    rblock->regs = regs;
    return rblock;
}

vm_cache_t *vm_cache_new(void) {
    vm_cache_t *cache = vm_malloc(sizeof(vm_cache_t));
    cache->keys = NULL;
    cache->values = NULL;
    cache->len = 0;
    cache->alloc = 0;
    return cache;
}

#if defined(VM_PRINT_RBLOCKS)
static const char *vm_tag_to_str(uint8_t tag) {
    static const char *table[] = {
        [VM_TAG_INIT] = "init",
        [VM_TAG_UNK] = "unk",
        [VM_TAG_NIL] = "nil",
        [VM_TAG_BOOL] = "bool",
        [VM_TAG_I8] = "i8",
        [VM_TAG_I16] = "i16",
        [VM_TAG_I32] = "i32",
        [VM_TAG_I64] = "i64",
        [VM_TAG_U8] = "u8",
        [VM_TAG_U16] = "u16",
        [VM_TAG_U32] = "u32",
        [VM_TAG_U64] = "u64",
        [VM_TAG_F32] = "f32",
        [VM_TAG_F64] = "f64",
        [VM_TAG_MEM] = "mem",
        [VM_TAG_FN] = "fn",
    };
    return table[tag];
}

static void vm_print_rblock(FILE *out, vm_rblock_t *rblock) {
    vm_block_t *block = rblock->block;
    fprintf(out, ".%zi(", block->id);
    for (size_t j = 0; j < block->nargs; j++) {
        if (j != 0) {
            fprintf(out, ", ");
        }
        fprintf(out, "r%zu : %s", block->args[j], vm_tag_to_str(rblock->regs[block->args[j]]));
    }
    fprintf(out, ")\n");
    for (size_t i = 0; i < block->len; i++) {
        if (block->instrs[i].op == VM_IOP_NOP) {
            continue;
        }
        fprintf(out, "    ");
        vm_print_instr(out, block->instrs[i]);
        fprintf(out, "\n");
    }
    if (block->branch.op != VM_BOP_FALL) {
        fprintf(out, "    ");
        vm_print_branch(out, block->branch);
        fprintf(out, "\n");
    } else {
        fprintf(out, "    <fall>\n");
    }
}
#endif

vm_opcode_t *vm_cache_get(vm_cache_t *cache, vm_rblock_t *rblock) {
    for (size_t i = 0; i < cache->len; i++) {
        if (vm_rblock_regs_match(rblock->regs, cache->keys[i])) {
            return cache->values[i];
        }
    }
#if defined(VM_PRINT_RBLOCKS)
    vm_print_rblock(stderr, rblock);
#endif
    return NULL;
}

void vm_cache_set(vm_cache_t *cache, vm_rblock_t *rblock, vm_opcode_t *value) {
    if (cache->len + 1 >= cache->alloc) {
        cache->alloc = cache->len * 2 + 1;
        cache->keys = vm_realloc(cache->keys, sizeof(uint8_t *) * cache->alloc);
        cache->values = vm_realloc(cache->values, sizeof(vm_opcode_t *) * cache->alloc);
    }
    cache->keys[cache->len] = rblock->regs;
    cache->values[cache->len] = value;
    cache->len += 1;
}

uint8_t *vm_rblock_regs_empty(void) {
    return vm_alloc0(sizeof(uint8_t) * VM_NREGS);
}

uint8_t *vm_rblock_regs_dup(uint8_t *regs) {
    uint8_t *ret = vm_alloc0(sizeof(uint8_t) * VM_NREGS);
    for (size_t i = 0; i < VM_NREGS; i++) {
        ret[i] = regs[i];
    }
    return ret;
}

bool vm_rblock_regs_match(uint8_t *a, uint8_t *b) {
    for (size_t i = 0; i < VM_NREGS; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

vm_instr_t vm_rblock_type_specialize_instr(uint8_t *types, vm_instr_t instr) {
    if (instr.tag == VM_TAG_UNK) {
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_REG) {
                instr.tag = types[instr.args[i].reg];
                return instr;
            }
        }
        for (size_t i = 0; instr.args[i].type != VM_ARG_NONE; i++) {
            if (instr.args[i].type == VM_ARG_NUM) {
                if (fmod(instr.args[i].num, 1) == 0) {
                    instr.tag = VM_TAG_I64;
                } else {
                    instr.tag = VM_TAG_F64;
                }
                return instr;
            }
        }
    }
    return instr;
}

vm_branch_t vm_rblock_type_specialize_branch(uint8_t *types, vm_branch_t branch) {
    if (branch.tag == VM_TAG_UNK) {
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_REG) {
                branch.tag = types[branch.args[i].reg];
                return branch;
            }
        }
        for (size_t i = 0; i < 2; i++) {
            if (branch.args[i].type == VM_ARG_NUM) {
                if (fmod(branch.args[i].num, 1) == 0) {
                    branch.tag = VM_TAG_I64;
                } else {
                    branch.tag = VM_TAG_F64;
                }
                return branch;
            }
        }
    }
    return branch;
}
