
#include "jit.h"
#include "../ir/ir.h"
#include "../lua/rt/api/std.h"
#include "../lua/rt/lua.h"
#include "../lua/rt/meta.h"
#include "../lua/rt/prim.h"
#include "../run/thread.h"
#include "../value/obj.h"
#include "../value/table.h"
#include "../vm/gc.h"

#if MX_JIT_SUPPORTED
#include <sys/mman.h>
#endif

const mx_x64_reg_t mx_jit_gprs[MX_JIT_NGPR] = {
    MX_X64_R13,
    MX_X64_R14,
    MX_X64_R15,
    MX_X64_RBP,
    MX_X64_RSI,
    MX_X64_RDI,
    MX_X64_R8,
    MX_X64_R9,
    MX_X64_R10,
    MX_X64_R11,
};

mx_jit_bit_t mx_jit_probe(const void *zeroed, size_t size) {
    const uint8_t *bytes = zeroed;
    mx_jit_bit_t bit = { .off = 0, .mask = 0 };
    for (size_t i = 0; i < size; i++) {
        if (bytes[i] != 0) {
            if (bit.mask != 0) {
                mx_fatal("jit: flags span bytes");
            }
            bit = (mx_jit_bit_t) { .off = (uint32_t)i, .mask = bytes[i] };
        }
    }
    return bit;
}

mx_jit_layout_t mx_jit_layout(void) {
    mx_obj_table_t t;
    memset(&t, 0, sizeof(t));
    t.is_plain = true;
    mx_jit_bit_t table_plain = mx_jit_probe(&t, sizeof(t));
    memset(&t, 0, sizeof(t));
    t.size = 0x3f;
    mx_jit_bit_t table_size = mx_jit_probe(&t, sizeof(t));
    memset(&t, 0, sizeof(t));
    t.has_int_hash = true;
    mx_jit_bit_t table_int_hash = mx_jit_probe(&t, sizeof(t));
    mx_obj_head_t h;
    memset(&h, 0, sizeof(h));
    h.is_old = true;
    h.is_black = true;
    mx_jit_bit_t head_barrier = mx_jit_probe(&h, sizeof(h));
    mx_resume_t r;
    memset(&r, 0, sizeof(r));
    r.has_cell_out = true;
    mx_jit_bit_t rec_cell_out = mx_jit_probe(&r, sizeof(r));
    mx_str_t s;
    memset(&s, 0, sizeof(s));
    s.is_rope = true;
    mx_jit_bit_t str_rope = mx_jit_probe(&s, sizeof(s));
    memset(&s, 0, sizeof(s));
    s.is_interned = true;
    mx_jit_bit_t str_interned = mx_jit_probe(&s, sizeof(s));
    mx_pending_t p;
    memset(&p, 0, sizeof(p));
    p.is_ok = true;
    mx_jit_bit_t pending_ok = mx_jit_probe(&p, sizeof(p));
    memset(&p, 0, sizeof(p));
    p.is_set = true;
    mx_jit_bit_t pending_set = mx_jit_probe(&p, sizeof(p));
    mx_t *vm = mx_mem_calloc(sizeof(mx_t));
    vm->is_hooked = true;
    mx_jit_bit_t vm_hooked = mx_jit_probe(vm, sizeof(mx_t));
    mx_mem_free(vm);
    if (table_plain.off != table_size.off || table_plain.mask <= table_size.mask) {
        mx_fatal("jit: unexpected table flags");
    }
    return (mx_jit_layout_t) {
        .pending_ok = pending_ok,
        .pending_set = pending_set,
        .vm_hooked = vm_hooked,
        .table_plain = table_plain,
        .table_size = table_size,
        .table_int_hash = table_int_hash,
        .head_barrier = head_barrier,
        .rec_cell_out = rec_cell_out,
        .str_rope = str_rope,
        .str_interned = str_interned,
    };
}

void mx_jit_epilogue(mx_x64_t *a) {
    mx_x64_alu_ri(a, MX_X64_ALU_ADD, MX_X64_RSP, MX_JIT_FRAME_BYTES);
    mx_x64_pop(a, MX_X64_R15);
    mx_x64_pop(a, MX_X64_R14);
    mx_x64_pop(a, MX_X64_R13);
    mx_x64_pop(a, MX_X64_R12);
    mx_x64_pop(a, MX_X64_RBP);
    mx_x64_pop(a, MX_X64_RBX);
    mx_x64_ret(a);
}

void mx_jit_stub_probe(mx_jit_t *jit, mx_x64_t *a) {
    const mx_jit_layout_t *layout = &jit->layout;
    int32_t key_kind_at = (int32_t)offsetof(mx_table_entry_t, key_kind);
    jit->slots[MX_JIT_SLOT_PROBE] = a->p;
    mx_x64_push(a, MX_X64_R8);
    mx_x64_push(a, MX_X64_R9);
    mx_x64_push(a, MX_X64_R10);
    mx_x64_test8_mi(a, MX_X64_RDX, (int32_t)layout->str_rope.off, layout->str_rope.mask);
    uint8_t *rope = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_load32(a, MX_X64_R8, MX_X64_RDX, (int32_t)offsetof(mx_str_t, hash));
    mx_x64_test_rr(a, MX_X64_R8, MX_X64_R8);
    uint8_t *no_hash = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_movzx8(a, MX_X64_RCX, MX_X64_RAX, (int32_t)layout->table_size.off);
    mx_x64_and32_ri(a, MX_X64_RCX, layout->table_size.mask);
    mx_x64_mov_ri(a, MX_X64_R9, MX_TABLE_HASH_MIN_CAP);
    mx_x64_shl32_cl(a, MX_X64_R9);
    mx_x64_lea32(a, MX_X64_R9, MX_X64_R9, -1);
    mx_x64_load(a, MX_X64_RAX, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX);
    uint8_t *empty = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_mov32_rr(a, MX_X64_RCX, MX_X64_R8);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_RCX, MX_X64_R9);
    uint8_t *loop = a->p;
    mx_x64_mov_rr(a, MX_X64_R10, MX_X64_RCX);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_R10, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_ADD, MX_X64_R10, MX_X64_RCX);
    mx_x64_lea_x(a, MX_X64_R10, MX_X64_RAX, MX_X64_R10, 0);
    mx_x64_cmp16_mi(a, MX_X64_R10, key_kind_at, MX_KIND_STRING);
    uint8_t *not_str = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_cmp_mr(a, MX_X64_R10, (int32_t)offsetof(mx_table_entry_t, key), MX_X64_RDX);
    uint8_t *found = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_cmp32_rm(a, MX_X64_R8, MX_X64_R10, (int32_t)offsetof(mx_table_entry_t, hash));
    uint8_t *same_hash = mx_jit_label(a, MX_X64_CC_E);
    uint8_t *next = a->p;
    mx_x64_lea32(a, MX_X64_RCX, MX_X64_RCX, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_RCX, MX_X64_R9);
    mx_x64_mov32_rr(a, MX_X64_R10, MX_X64_R8);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_R10, MX_X64_R9);
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R10, MX_X64_RCX);
    mx_x64_jcc(a, MX_X64_CC_NE, loop);
    uint8_t *wrapped = mx_jit_label_jmp(a);
    mx_x64_patch(not_str, a->p);
    mx_x64_cmp16_mi(a, MX_X64_R10, key_kind_at, MX_KIND_NIL);
    mx_x64_jcc(a, MX_X64_CC_NE, next);
    mx_x64_patch(wrapped, a->p);
    mx_x64_patch(empty, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    uint8_t *done_absent = mx_jit_label_jmp(a);
    mx_x64_patch(found, a->p);
    mx_x64_mov_rr(a, MX_X64_RCX, MX_X64_R10);
    mx_x64_mov_ri(a, MX_X64_RAX, 1);
    uint8_t *done_found = mx_jit_label_jmp(a);
    mx_x64_patch(rope, a->p);
    mx_x64_patch(no_hash, a->p);
    mx_x64_patch(same_hash, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 2);
    mx_x64_patch(done_absent, a->p);
    mx_x64_patch(done_found, a->p);
    mx_x64_pop(a, MX_X64_R10);
    mx_x64_pop(a, MX_X64_R9);
    mx_x64_pop(a, MX_X64_R8);
    mx_x64_ret(a);
}

void mx_jit_stub_leave(mx_x64_t *a) {
    mx_x64_store(a, MX_X64_R12, (int32_t)offsetof(mx_t, jit.rb), MX_X64_RCX);
    mx_x64_mov_rr(a, MX_X64_RAX, MX_X64_R8);
    mx_x64_mov_rr(a, MX_X64_RDX, MX_X64_RBX);
    mx_jit_epilogue(a);
}

void mx_jit_stub_frame(mx_jit_t *jit, mx_x64_t *a, mx_jit_slot_t tramp, mx_jit_slot_t call, mx_x64_reg_t second) {
    jit->slots[tramp] = a->p;
    mx_x64_mov_rr(a, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_rr(a, MX_X64_RSI, second);
    mx_x64_push(a, MX_X64_RDX);
    mx_x64_push(a, MX_X64_RCX);
    mx_x64_call_rip(a, &jit->slots[call]);
    mx_x64_pop(a, MX_X64_RCX);
    mx_x64_pop(a, MX_X64_R8);
    mx_x64_mov_rr(a, MX_X64_RBX, MX_X64_RDX);
    mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX);
    uint8_t *exit = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_jmp_m(a, MX_X64_RAX, 8);
    mx_x64_patch(exit, a->p);
    mx_jit_stub_leave(a);
}

void mx_jit_stub_code(mx_jit_t *jit, mx_x64_t *a, mx_jit_slot_t tramp, mx_jit_slot_t call, bool has_base) {
    jit->slots[tramp] = a->p;
    mx_x64_mov_rr(a, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_rr(a, MX_X64_RSI, MX_X64_RBX);
    mx_x64_push(a, MX_X64_RDX);
    mx_x64_push(a, MX_X64_R8);
    mx_x64_call_rip(a, &jit->slots[call]);
    mx_x64_pop(a, MX_X64_RCX);
    mx_x64_pop(a, MX_X64_R8);
    if (has_base) {
        mx_x64_mov_rr(a, MX_X64_RBX, MX_X64_RDX);
    }
    mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX);
    uint8_t *exit = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_jmp_m(a, MX_X64_RAX, 8);
    mx_x64_patch(exit, a->p);
    mx_jit_stub_leave(a);
}

void mx_jit_stub_lookup_ref(mx_jit_t *jit, mx_x64_t *a) {
    const mx_jit_layout_t *layout = &jit->layout;
    jit->slots[MX_JIT_SLOT_LOOKUP_REF] = a->p;
    mx_x64_test8_mi(a, MX_X64_RAX, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    uint8_t *not_plain = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_push(a, MX_X64_RSI);
    mx_x64_push(a, MX_X64_R8);
    mx_x64_push(a, MX_X64_R9);
    mx_x64_push(a, MX_X64_R10);
    mx_x64_push(a, MX_X64_R11);
    mx_x64_mov_rr(a, MX_X64_R11, MX_X64_RAX);
    mx_x64_mov_rr(a, MX_X64_R8, MX_X64_RCX);
    mx_x64_movzx8(a, MX_X64_RCX, MX_X64_RAX, (int32_t)layout->table_size.off);
    mx_x64_and32_ri(a, MX_X64_RCX, layout->table_size.mask);
    mx_x64_mov_ri(a, MX_X64_R9, MX_TABLE_HASH_MIN_CAP);
    mx_x64_shl32_cl(a, MX_X64_R9);
    mx_x64_lea32(a, MX_X64_R9, MX_X64_R9, -1);
    mx_x64_load(a, MX_X64_RAX, MX_X64_R11, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX);
    uint8_t *empty = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_mov32_rr(a, MX_X64_RCX, MX_X64_RDX);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHR, MX_X64_RCX, MX_TABLE_HASH_PTR_SHIFT);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_RCX, MX_X64_R9);
    uint8_t *loop = a->p;
    mx_x64_mov_rr(a, MX_X64_R10, MX_X64_RCX);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_R10, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_ADD, MX_X64_R10, MX_X64_RCX);
    mx_x64_lea_x(a, MX_X64_R10, MX_X64_RAX, MX_X64_R10, 0);
    mx_x64_movzx16(a, MX_X64_RSI, MX_X64_R10, (int32_t)offsetof(mx_table_entry_t, key_kind));
    mx_x64_test_rr(a, MX_X64_RSI, MX_X64_RSI);
    uint8_t *hole = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_RSI, MX_X64_R8);
    uint8_t *other_kind = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_cmp_mr(a, MX_X64_R10, (int32_t)offsetof(mx_table_entry_t, key), MX_X64_RDX);
    uint8_t *found = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_patch(other_kind, a->p);
    mx_x64_lea32(a, MX_X64_RCX, MX_X64_RCX, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_RCX, MX_X64_R9);
    mx_x64_mov32_rr(a, MX_X64_R10, MX_X64_RDX);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHR, MX_X64_R10, MX_TABLE_HASH_PTR_SHIFT);
    mx_x64_alu_rr(a, MX_X64_ALU_AND, MX_X64_R10, MX_X64_R9);
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R10, MX_X64_RCX);
    mx_x64_jcc(a, MX_X64_CC_NE, loop);
    mx_x64_patch(hole, a->p);
    mx_x64_patch(empty, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    mx_x64_cmp_mi(a, MX_X64_R11, (int32_t)offsetof(mx_obj_table_t, meta), 0);
    uint8_t *no_meta = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_mov_ri(a, MX_X64_RAX, 2);
    mx_x64_patch(no_meta, a->p);
    uint8_t *out = mx_jit_label_jmp(a);
    mx_x64_patch(found, a->p);
    mx_x64_mov_rr(a, MX_X64_RCX, MX_X64_R10);
    mx_x64_mov_ri(a, MX_X64_RAX, 1);
    mx_x64_patch(out, a->p);
    mx_x64_pop(a, MX_X64_R11);
    mx_x64_pop(a, MX_X64_R10);
    mx_x64_pop(a, MX_X64_R9);
    mx_x64_pop(a, MX_X64_R8);
    mx_x64_pop(a, MX_X64_RSI);
    mx_x64_ret(a);
    mx_x64_patch(not_plain, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 2);
    mx_x64_ret(a);
}

void mx_jit_stub_lookup(mx_t *vm, mx_x64_t *a) {
    mx_jit_t *jit = &vm->jit;
    const mx_jit_bit_t *plain = &jit->layout.table_plain;
    jit->slots[MX_JIT_SLOT_LOOKUP] = a->p;
    mx_x64_test8_mi(a, MX_X64_RAX, (int32_t)plain->off, plain->mask);
    uint8_t *not_plain = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_push(a, MX_X64_RAX);
    mx_x64_push(a, MX_X64_RDX);
    mx_x64_call_rip(a, &jit->slots[MX_JIT_SLOT_PROBE]);
    mx_x64_pop(a, MX_X64_RDX);
    mx_x64_cmp32_ri(a, MX_X64_RAX, 0);
    uint8_t *absent = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_alu_ri(a, MX_X64_ALU_ADD, MX_X64_RSP, 8);
    mx_x64_ret(a);
    mx_x64_patch(absent, a->p);
    mx_x64_pop(a, MX_X64_RAX);
    mx_x64_cmp_mi(a, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, meta), 0);
    uint8_t *no_meta = mx_jit_label(a, MX_X64_CC_E);
    mx_obj_t handler = vm->kinds.ptr[vm->kinds.table].ops[MX_OP_GET];
    if (mx_obj_is_ffi(handler) && mx_obj_get_ffi(handler) == vm->meta_get.fn) {
        int32_t ops_at = (int32_t)(vm->kinds.table * sizeof(mx_kind_info_t) + offsetof(mx_kind_info_t, ops) + MX_OP_GET * sizeof(mx_obj_t) + offsetof(mx_obj_t, val));
        mx_x64_load(a, MX_X64_RCX, MX_X64_R12, (int32_t)(offsetof(mx_t, kinds) + offsetof(mx_kinds_t, ptr)));
        mx_x64_push(a, MX_X64_RDX);
        mx_x64_mov_ri(a, MX_X64_RDX, handler.val.bits);
        mx_x64_cmp_mr(a, MX_X64_RCX, ops_at, MX_X64_RDX);
        mx_x64_pop(a, MX_X64_RDX);
        uint8_t *changed = mx_jit_label(a, MX_X64_CC_NE);
        mx_x64_call_rip(a, &jit->slots[MX_JIT_SLOT_CHAIN]);
        mx_x64_ret(a);
        mx_x64_patch(changed, a->p);
    }
    mx_x64_patch(not_plain, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 2);
    mx_x64_ret(a);
    mx_x64_patch(no_meta, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    mx_x64_ret(a);
}

void mx_jit_stub_ret_ways(mx_jit_t *jit, mx_x64_t *a) {
    jit->slots[MX_JIT_SLOT_RET_WAYS] = a->p;
    mx_x64_load(a, MX_X64_RCX, MX_X64_RDX, (int32_t)(offsetof(mx_resume_t, cache) + offsetof(mx_resume_cache_t, more)));
    mx_x64_test_rr(a, MX_X64_RCX, MX_X64_RCX);
    uint8_t *none = mx_jit_label(a, MX_X64_CC_E);
    uint8_t *hits[MX_RESUME_WAYS - 1];
    for (uint32_t i = 0; i < MX_RESUME_WAYS - 1; i++) {
        mx_x64_cmp_mr(a, MX_X64_RCX, (int32_t)(sizeof(uintptr_t) * 2 * i), MX_X64_RAX);
        hits[i] = mx_jit_label(a, MX_X64_CC_E);
    }
    mx_x64_patch(none, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    mx_x64_ret(a);
    for (uint32_t i = 0; i < MX_RESUME_WAYS - 1; i++) {
        mx_x64_patch(hits[i], a->p);
        mx_x64_load(a, MX_X64_RAX, MX_X64_RCX, (int32_t)(sizeof(uintptr_t) * (2 * i + 1)));
        mx_x64_ret(a);
    }
}

void mx_jit_stub_deliver(mx_jit_t *jit, mx_x64_t *a, uint32_t want) {
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    int32_t cache_at = (int32_t)offsetof(mx_resume_t, cache);
    jit->slots[MX_JIT_SLOT_DELIVER2 + want - 2] = a->p;
    mx_x64_cmp32_mi(a, MX_X64_R12, (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len)), (int32_t)want);
    uint8_t *few = mx_jit_label(a, MX_X64_CC_B);
    mx_x64_movzx16(a, MX_X64_RAX, MX_X64_RSI, kind_at);
    for (uint32_t i = 1; i < want; i++) {
        mx_x64_movzx16(a, MX_X64_RCX, MX_X64_RSI, (int32_t)(i * sizeof(mx_obj_t)) + kind_at);
        mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_RCX, (uint8_t)(16 * i));
        mx_x64_alu_rr(a, MX_X64_ALU_OR, MX_X64_RAX, MX_X64_RCX);
    }
    mx_x64_mov_ri(a, MX_X64_RCX, ((uint64_t)(want + 1) << 48) | MX_RUN_KEY_RESUMED);
    mx_x64_alu_rr(a, MX_X64_ALU_OR, MX_X64_RAX, MX_X64_RCX);
    mx_x64_cmp_mr(a, MX_X64_RDX, cache_at + (int32_t)offsetof(mx_resume_cache_t, key), MX_X64_RAX);
    uint8_t *first = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_call_rip(a, &jit->slots[MX_JIT_SLOT_RET_WAYS]);
    mx_x64_test_rr(a, MX_X64_RAX, MX_X64_RAX);
    uint8_t *none = mx_jit_label(a, MX_X64_CC_E);
    uint8_t *have = mx_jit_label_jmp(a);
    mx_x64_patch(first, a->p);
    mx_x64_load(a, MX_X64_RAX, MX_X64_RDX, cache_at + (int32_t)offsetof(mx_resume_cache_t, code));
    mx_x64_patch(have, a->p);
    mx_x64_movzx16(a, MX_X64_RCX, MX_X64_RDX, (int32_t)offsetof(mx_resume_t, out));
    mx_x64_lea_x(a, MX_X64_RCX, MX_X64_RBX, MX_X64_RCX, 0);
    for (uint32_t i = 0; i < want; i++) {
        mx_x64_load(a, MX_X64_RDX, MX_X64_RSI, (int32_t)(i * sizeof(mx_obj_t)) + val_at);
        mx_x64_store(a, MX_X64_RCX, (int32_t)(i * sizeof(mx_val_t)), MX_X64_RDX);
    }
    mx_x64_ret(a);
    mx_x64_patch(few, a->p);
    mx_x64_patch(none, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    mx_x64_ret(a);
}

void mx_jit_stub_next(mx_jit_t *jit, mx_x64_t *a) {
    const mx_jit_layout_t *layout = &jit->layout;
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    jit->slots[MX_JIT_SLOT_NEXT] = a->p;
    mx_x64_push(a, MX_X64_RSI);
    mx_x64_push(a, MX_X64_R8);
    mx_x64_push(a, MX_X64_R9);
    mx_x64_push(a, MX_X64_R10);
    mx_x64_lea(a, MX_X64_RSI, MX_X64_RSP, 40);
    mx_x64_test_rr(a, MX_X64_RCX, MX_X64_RCX);
    uint8_t *is_start = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_cmp32_ri(a, MX_X64_RCX, MX_KIND_INT);
    uint8_t *not_int = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_lea(a, MX_X64_R8, MX_X64_RDX, -1);
    mx_x64_load32(a, MX_X64_R9, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, array_len));
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R8, MX_X64_R9);
    uint8_t *hint_int = mx_jit_label(a, MX_X64_CC_AE);
    mx_x64_mov_rr(a, MX_X64_R8, MX_X64_RDX);
    uint8_t *scan_mid = mx_jit_label_jmp(a);
    mx_x64_patch(is_start, a->p);
    mx_x64_mov_ri(a, MX_X64_R8, 0);
    mx_x64_load32(a, MX_X64_R9, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, array_len));
    mx_x64_patch(scan_mid, a->p);
    mx_x64_load(a, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, array));
    uint8_t *array_loop = a->p;
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R8, MX_X64_R9);
    uint8_t *array_done = mx_jit_label(a, MX_X64_CC_AE);
    mx_x64_mov_rr(a, MX_X64_R10, MX_X64_R8);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_R10, 4);
    mx_x64_alu_rr(a, MX_X64_ALU_ADD, MX_X64_R10, MX_X64_RCX);
    mx_x64_lea(a, MX_X64_R8, MX_X64_R8, 1);
    mx_x64_movzx16(a, MX_X64_RDX, MX_X64_R10, kind_at);
    mx_x64_test_rr(a, MX_X64_RDX, MX_X64_RDX);
    mx_x64_jcc(a, MX_X64_CC_E, array_loop);
    mx_x64_store16_i(a, MX_X64_RSI, kind_at, MX_KIND_INT);
    mx_x64_store(a, MX_X64_RSI, val_at, MX_X64_R8);
    mx_x64_store(a, MX_X64_RSI, (int32_t)sizeof(mx_obj_t) + kind_at, MX_X64_RDX);
    mx_x64_load(a, MX_X64_RDX, MX_X64_R10, val_at);
    mx_x64_store(a, MX_X64_RSI, (int32_t)sizeof(mx_obj_t) + val_at, MX_X64_RDX);
    uint8_t *done_array = mx_jit_label_jmp(a);
    mx_x64_patch(array_done, a->p);
    mx_x64_mov_ri(a, MX_X64_R8, 0);
    uint8_t *scan_hash = mx_jit_label_jmp(a);
    mx_x64_patch(hint_int, a->p);
    mx_x64_patch(not_int, a->p);
    mx_x64_load(a, MX_X64_R8, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, hidden));
    mx_x64_test_rr(a, MX_X64_R8, MX_X64_R8);
    uint8_t *no_hint = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_load(a, MX_X64_R10, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(a, MX_X64_R10, MX_X64_R10);
    uint8_t *no_entries = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_mov_rr(a, MX_X64_R9, MX_X64_RCX);
    mx_x64_movzx8(a, MX_X64_RCX, MX_X64_RAX, (int32_t)layout->table_size.off);
    mx_x64_and32_ri(a, MX_X64_RCX, layout->table_size.mask);
    mx_x64_push(a, MX_X64_R11);
    mx_x64_mov_ri(a, MX_X64_R11, MX_TABLE_HASH_MIN_CAP);
    mx_x64_shl32_cl(a, MX_X64_R11);
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R8, MX_X64_R11);
    mx_x64_pop(a, MX_X64_R11);
    uint8_t *big_hint = mx_jit_label(a, MX_X64_CC_A);
    mx_x64_mov_rr(a, MX_X64_RCX, MX_X64_R8);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_RCX, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_ADD, MX_X64_RCX, MX_X64_R8);
    mx_x64_lea_x(a, MX_X64_RCX, MX_X64_R10, MX_X64_RCX, 0);
    mx_x64_cmp_mr(a, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key) - (int32_t)sizeof(mx_table_entry_t), MX_X64_RDX);
    uint8_t *other_key = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_movzx16(a, MX_X64_RDX, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key_kind) - (int32_t)sizeof(mx_table_entry_t));
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_RDX, MX_X64_R9);
    uint8_t *other_kind = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_patch(scan_hash, a->p);
    mx_x64_load(a, MX_X64_R10, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(a, MX_X64_R10, MX_X64_R10);
    uint8_t *no_entries2 = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_movzx8(a, MX_X64_RCX, MX_X64_RAX, (int32_t)layout->table_size.off);
    mx_x64_and32_ri(a, MX_X64_RCX, layout->table_size.mask);
    mx_x64_mov_ri(a, MX_X64_R9, MX_TABLE_HASH_MIN_CAP);
    mx_x64_shl32_cl(a, MX_X64_R9);
    uint8_t *hash_loop = a->p;
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_R8, MX_X64_R9);
    uint8_t *hash_done = mx_jit_label(a, MX_X64_CC_AE);
    mx_x64_mov_rr(a, MX_X64_RCX, MX_X64_R8);
    mx_x64_lea(a, MX_X64_R8, MX_X64_R8, 1);
    mx_x64_mov_rr(a, MX_X64_RDX, MX_X64_RCX);
    mx_x64_shift_i(a, MX_X64_SHIFT_SHL, MX_X64_RCX, 1);
    mx_x64_alu_rr(a, MX_X64_ALU_ADD, MX_X64_RCX, MX_X64_RDX);
    mx_x64_lea_x(a, MX_X64_RCX, MX_X64_R10, MX_X64_RCX, 0);
    mx_x64_movzx16(a, MX_X64_RDX, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key_kind));
    mx_x64_test_rr(a, MX_X64_RDX, MX_X64_RDX);
    mx_x64_jcc(a, MX_X64_CC_E, hash_loop);
    mx_x64_cmp16_mi(a, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind), MX_KIND_NIL);
    mx_x64_jcc(a, MX_X64_CC_E, hash_loop);
    mx_x64_store(a, MX_X64_RAX, (int32_t)offsetof(mx_obj_table_t, hidden), MX_X64_R8);
    mx_x64_store(a, MX_X64_RSI, kind_at, MX_X64_RDX);
    mx_x64_load(a, MX_X64_RDX, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key));
    mx_x64_store(a, MX_X64_RSI, val_at, MX_X64_RDX);
    mx_x64_movzx16(a, MX_X64_RDX, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind));
    mx_x64_store(a, MX_X64_RSI, (int32_t)sizeof(mx_obj_t) + kind_at, MX_X64_RDX);
    mx_x64_load(a, MX_X64_RDX, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val));
    mx_x64_store(a, MX_X64_RSI, (int32_t)sizeof(mx_obj_t) + val_at, MX_X64_RDX);
    mx_x64_patch(done_array, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 1);
    uint8_t *out = mx_jit_label_jmp(a);
    mx_x64_patch(no_hint, a->p);
    mx_x64_patch(no_entries, a->p);
    mx_x64_patch(big_hint, a->p);
    mx_x64_patch(other_key, a->p);
    mx_x64_patch(other_kind, a->p);
    mx_x64_patch(no_entries2, a->p);
    mx_x64_patch(hash_done, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    mx_x64_patch(out, a->p);
    mx_x64_pop(a, MX_X64_R10);
    mx_x64_pop(a, MX_X64_R9);
    mx_x64_pop(a, MX_X64_R8);
    mx_x64_pop(a, MX_X64_RSI);
    mx_x64_ret(a);
}

void mx_jit_stub_chain(mx_jit_t *jit, mx_x64_t *a) {
    const mx_jit_layout_t *layout = &jit->layout;
    int32_t val_kind_at = (int32_t)offsetof(mx_table_entry_t, val_kind);
    int32_t meta_at = (int32_t)offsetof(mx_obj_table_t, meta);
    jit->slots[MX_JIT_SLOT_CHAIN] = a->p;
    mx_x64_push(a, MX_X64_R10);
    mx_x64_push(a, MX_X64_R11);
    mx_x64_push(a, MX_X64_RSI);
    mx_x64_mov_rr(a, MX_X64_R11, MX_X64_RAX);
    mx_x64_mov_rr(a, MX_X64_R10, MX_X64_RDX);
    mx_x64_mov_ri(a, MX_X64_RSI, MX_RUN_MAX_CHAIN);
    uint8_t *loop = a->p;
    mx_x64_load(a, MX_X64_RAX, MX_X64_R11, meta_at);
    mx_x64_load(a, MX_X64_RDX, MX_X64_R12, (int32_t)(offsetof(mx_t, meta_get) + offsetof(mx_meta_std_t, key) + offsetof(mx_obj_t, val)));
    mx_x64_call_rip(a, &jit->slots[MX_JIT_SLOT_PROBE]);
    mx_x64_cmp32_ri(a, MX_X64_RAX, 1);
    uint8_t *no_handler = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_movzx16(a, MX_X64_RAX, MX_X64_RCX, val_kind_at);
    mx_x64_movzx16(a, MX_X64_RDX, MX_X64_R12, (int32_t)(offsetof(mx_t, kinds) + offsetof(mx_kinds_t, table)));
    mx_x64_alu_rr(a, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RDX);
    uint8_t *other_kind = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_load(a, MX_X64_R11, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val));
    mx_x64_test8_mi(a, MX_X64_R11, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    uint8_t *not_plain = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_mov_rr(a, MX_X64_RAX, MX_X64_R11);
    mx_x64_mov_rr(a, MX_X64_RDX, MX_X64_R10);
    mx_x64_call_rip(a, &jit->slots[MX_JIT_SLOT_PROBE]);
    mx_x64_cmp32_ri(a, MX_X64_RAX, 1);
    uint8_t *not_found = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_cmp16_mi(a, MX_X64_RCX, val_kind_at, MX_KIND_NIL);
    uint8_t *nil_value = mx_jit_label(a, MX_X64_CC_E);
    uint8_t *done_found = mx_jit_label_jmp(a);
    mx_x64_patch(not_found, a->p);
    mx_x64_cmp32_ri(a, MX_X64_RAX, 0);
    uint8_t *give_up_probe = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_patch(nil_value, a->p);
    mx_x64_cmp_mi(a, MX_X64_R11, meta_at, 0);
    uint8_t *ended = mx_jit_label(a, MX_X64_CC_E);
    mx_x64_alu_ri(a, MX_X64_ALU_SUB, MX_X64_RSI, 1);
    mx_x64_jcc(a, MX_X64_CC_NE, loop);
    uint8_t *too_long = mx_jit_label_jmp(a);
    mx_x64_patch(other_kind, a->p);
    mx_x64_cmp32_ri(a, MX_X64_RAX, MX_KIND_NIL);
    uint8_t *not_nil = mx_jit_label(a, MX_X64_CC_NE);
    uint8_t *nil_handler = mx_jit_label_jmp(a);
    mx_x64_patch(no_handler, a->p);
    mx_x64_cmp32_ri(a, MX_X64_RAX, 0);
    uint8_t *give_up_handler = mx_jit_label(a, MX_X64_CC_NE);
    mx_x64_patch(nil_handler, a->p);
    mx_x64_patch(ended, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 0);
    uint8_t *done_absent = mx_jit_label_jmp(a);
    mx_x64_patch(not_nil, a->p);
    mx_x64_patch(not_plain, a->p);
    mx_x64_patch(give_up_probe, a->p);
    mx_x64_patch(give_up_handler, a->p);
    mx_x64_patch(too_long, a->p);
    mx_x64_mov_ri(a, MX_X64_RAX, 2);
    mx_x64_patch(done_absent, a->p);
    uint8_t *out = a->p;
    mx_x64_pop(a, MX_X64_RSI);
    mx_x64_pop(a, MX_X64_R11);
    mx_x64_pop(a, MX_X64_R10);
    mx_x64_ret(a);
    uint8_t *found = a->p;
    mx_x64_patch(done_found, found);
    mx_x64_mov_ri(a, MX_X64_RAX, 1);
    mx_x64_jmp(a, out);
}

void mx_jit_stubs(mx_t *vm) {
    mx_jit_t *jit = &vm->jit;
    jit->slots = (void **)jit->mem;
    jit->slots[MX_JIT_SLOT_HOP] = (void *)mx_jit_rt_hop;
    jit->slots[MX_JIT_SLOT_CMP] = (void *)mx_jit_rt_cmp;
    jit->slots[MX_JIT_SLOT_SET] = (void *)mx_jit_rt_set;
    jit->slots[MX_JIT_SLOT_GET_SK] = (void *)mx_jit_rt_get_sk;
    jit->slots[MX_JIT_SLOT_GET_IR] = (void *)mx_jit_rt_get_ir;
    jit->slots[MX_JIT_SLOT_GET_IK] = (void *)mx_jit_rt_get_ik;
    jit->slots[MX_JIT_SLOT_GET_VAR] = (void *)mx_jit_rt_get_var;
    jit->slots[MX_JIT_SLOT_GET_METHOD] = (void *)mx_jit_rt_get_method;
    jit->slots[MX_JIT_SLOT_GET_UPVAL] = (void *)mx_jit_rt_get_upval;
    jit->slots[MX_JIT_SLOT_CALL_FAST] = (void *)mx_jit_rt_call_fast;
    jit->slots[MX_JIT_SLOT_TABLE_NEW] = (void *)mx_jit_rt_table_new;
    jit->slots[MX_JIT_SLOT_CONCAT] = (void *)mx_jit_rt_concat;
    jit->slots[MX_JIT_SLOT_CLOSURE] = (void *)mx_jit_rt_closure;
    jit->slots[MX_JIT_SLOT_CLOSE] = (void *)mx_jit_rt_close;
    jit->slots[MX_JIT_SLOT_EQ_STR] = (void *)mx_jit_rt_eq_str;
    jit->slots[MX_JIT_SLOT_LINK] = (void *)mx_jit_rt_link;
    jit->slots[MX_JIT_SLOT_DELIVER] = (void *)mx_jit_rt_deliver;
    jit->slots[MX_JIT_SLOT_FFI] = (void *)mx_jit_rt_ffi;
    jit->slots[MX_JIT_SLOT_FFI_AFTER] = (void *)mx_jit_rt_ffi_after;
    jit->slots[MX_JIT_SLOT_RETURN] = (void *)mx_jit_rt_return;
    jit->slots[MX_JIT_SLOT_CALL_LUA] = (void *)mx_jit_rt_call_lua;
    jit->slots[MX_JIT_SLOT_TAILCALL_LUA] = (void *)mx_jit_rt_tailcall_lua;
    jit->slots[MX_JIT_SLOT_META_CMP] = (void *)mx_jit_rt_meta_cmp;
    jit->slots[MX_JIT_SLOT_CELL] = (void *)mx_jit_rt_cell;
    jit->slots[MX_JIT_SLOT_UNPACK] = (void *)mx_jit_rt_unpack;
    mx_x64_t a = { .p = jit->mem + sizeof(void *) * MX_JIT_SLOT_COUNT };
    uint8_t *enter = a.p;
    mx_x64_push(&a, MX_X64_RBX);
    mx_x64_push(&a, MX_X64_RBP);
    mx_x64_push(&a, MX_X64_R12);
    mx_x64_push(&a, MX_X64_R13);
    mx_x64_push(&a, MX_X64_R14);
    mx_x64_push(&a, MX_X64_R15);
    mx_x64_alu_ri(&a, MX_X64_ALU_SUB, MX_X64_RSP, MX_JIT_FRAME_BYTES);
    mx_x64_mov_rr(&a, MX_X64_RBX, MX_X64_RDI);
    mx_x64_mov_rr(&a, MX_X64_R12, MX_X64_RSI);
    mx_x64_jmp_r(&a, MX_X64_RDX);
    jit->enter = (mx_jit_enter_t *)(void *)enter;
    jit->exit = a.p;
    mx_x64_store(&a, MX_X64_R12, (int32_t)offsetof(mx_t, jit.rb), MX_X64_RDX);
    mx_x64_load(&a, MX_X64_RCX, MX_X64_RDX, (int32_t)offsetof(mx_version_t, code));
    mx_x64_lea_x(&a, MX_X64_RAX, MX_X64_RCX, MX_X64_RAX, 0);
    mx_x64_mov_rr(&a, MX_X64_RDX, MX_X64_RBX);
    mx_jit_epilogue(&a);
    jit->back = (uintptr_t)a.p;
    mx_x64_load(&a, MX_X64_RDX, MX_X64_RAX, -8);
    mx_x64_store(&a, MX_X64_R12, (int32_t)offsetof(mx_t, jit.rb), MX_X64_RDX);
    mx_x64_mov_rr(&a, MX_X64_RDX, MX_X64_RBX);
    mx_jit_epilogue(&a);
    jit->link = a.p;
    mx_x64_mov_rr(&a, MX_X64_RDI, MX_X64_R12);
    mx_x64_call_rip(&a, &jit->slots[MX_JIT_SLOT_LINK]);
    mx_x64_jmp_m(&a, MX_X64_RAX, 8);
    mx_jit_stub_probe(jit, &a);
    mx_jit_stub_chain(jit, &a);
    mx_jit_stub_ret_ways(jit, &a);
    mx_jit_stub_deliver(jit, &a, 2);
    mx_jit_stub_deliver(jit, &a, 3);
    mx_jit_stub_next(jit, &a);
    mx_jit_stub_lookup_ref(jit, &a);
    mx_jit_stub_frame(jit, &a, MX_JIT_SLOT_T_FFI, MX_JIT_SLOT_FFI, MX_X64_RBX);
    mx_jit_stub_frame(jit, &a, MX_JIT_SLOT_T_FFI_AFTER, MX_JIT_SLOT_FFI_AFTER, MX_X64_R13);
    mx_jit_stub_frame(jit, &a, MX_JIT_SLOT_T_CALL_LUA, MX_JIT_SLOT_CALL_LUA, MX_X64_RBX);
    mx_jit_stub_frame(jit, &a, MX_JIT_SLOT_T_TAILCALL_LUA, MX_JIT_SLOT_TAILCALL_LUA, MX_X64_RBX);
    mx_jit_stub_code(jit, &a, MX_JIT_SLOT_T_RETURN, MX_JIT_SLOT_RETURN, true);
    for (uint32_t i = 0; i <= MX_JIT_SLOT_GET_UPVAL - MX_JIT_SLOT_GET_SK; i++) {
        mx_jit_stub_code(jit, &a, (mx_jit_slot_t)(MX_JIT_SLOT_T_GET_SK + i), (mx_jit_slot_t)(MX_JIT_SLOT_GET_SK + i), false);
    }
    jit->hot = a.p;
    jit->cold = jit->mem + MX_JIT_COLD_OFFSET;
    mx_x64_t probe = { .p = jit->hot };
    mx_x64_cmp8_mi(&probe, MX_X64_R12, (int32_t)offsetof(mx_t, interrupt), 0);
    mx_x64_jcc(&probe, MX_X64_CC_NE, jit->hot);
    jit->poll_len = (uint32_t)(probe.p - jit->hot);
}

void mx_jit_hop_map(mx_t *vm) {
    mx_jit_t *jit = &vm->jit;
    for (mx_hop_t hop = 0; hop < MX_HOP_COUNT; hop++) {
        uintptr_t word = (uintptr_t)vm->hops[hop];
        uint32_t at = (uint32_t)((word >> 4) * UINT64_C(0x9e3779b97f4a7c15) >> 40) & 511;
        while (jit->hop_keys[at] != 0) {
            at = (at + 1) & 511;
        }
        jit->hop_keys[at] = word;
        jit->hop_vals[at] = (uint8_t)hop;
    }
}

mx_hop_t mx_jit_hop_of(const mx_t *vm, uintptr_t word) {
    const mx_jit_t *jit = &vm->jit;
    uint32_t at = (uint32_t)((word >> 4) * UINT64_C(0x9e3779b97f4a7c15) >> 40) & 511;
    while (jit->hop_keys[at] != word) {
        at = (at + 1) & 511;
    }
    return jit->hop_vals[at];
}

void mx_jit_init(mx_t *vm) {
#if MX_JIT_SUPPORTED
    const char *mode = getenv("MX_JIT");
    if (mode != NULL && strcmp(mode, "0") == 0) {
        return;
    }
    void *mem = mmap(NULL, MX_JIT_REGION_BYTES, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (mem == MAP_FAILED) {
        return;
    }
    vm->jit.mem = mem;
    const char *threshold = getenv("MX_JIT_THRESHOLD");
    if (threshold != NULL) {
        vm->jit.threshold = (uint32_t)strtoul(threshold, NULL, 10);
    } else {
        vm->jit.threshold = MX_JIT_THRESHOLD_DEFAULT;
    }
    vm->jit.layout = mx_jit_layout();
    vm->jit.labels = mx_mem_alloc(sizeof(mx_jit_label_t) * (MX_JIT_STITCH_MAX + 2));
    vm->jit.pend = mx_mem_alloc(sizeof(mx_jit_pend_t) * (MX_JIT_STITCH_MAX + 2));
    for (uint64_t s = 0; s < 64; s++) {
        uint64_t cap = (uint64_t)MX_TABLE_HASH_MIN_CAP << (s & 31);
        if (cap <= MX_TABLE_HASH_MIN_CAP) {
            vm->jit.room[s] = UINT32_MAX;
        } else {
            vm->jit.room[s] = cap * MX_TABLE_LOAD_NUM / MX_TABLE_LOAD_DEN;
        }
    }
    mx_jit_hop_map(vm);
    mx_jit_stubs(vm);
#else
    (void)vm;
#endif
}

void mx_jit_free(mx_t *vm) {
#if MX_JIT_SUPPORTED
#if MX_GC_STAT
    const char *dump = getenv("MX_JIT_DUMP");
    if (dump != NULL && vm->jit.mem != NULL) {
        FILE *out = fopen(dump, "wb");
        if (out != NULL) {
            fwrite(vm->jit.mem, 1, (size_t)(vm->jit.hot - vm->jit.mem), out);
            fclose(out);
        }
    }
#endif
    if (vm->jit.mem != NULL) {
        mx_mem_free(vm->jit.labels);
        mx_mem_free(vm->jit.pend);
        munmap(vm->jit.mem, MX_JIT_REGION_BYTES);
    }
#else
    (void)vm;
#endif
}

int32_t mx_jit_disp(uint32_t reg) {
    return (int32_t)(reg * sizeof(mx_val_t));
}

int32_t mx_jit_home_disp(const mx_jit_cc_t *cc, uint32_t abs) {
    return ((int32_t)abs - (int32_t)cc->off) * (int32_t)sizeof(mx_val_t);
}

bool mx_jit_is_caller_saved(mx_x64_reg_t r) {
    return r != MX_X64_R13 && r != MX_X64_R14 && r != MX_X64_R15 && r != MX_X64_RBP;
}

uint8_t mx_jit_find(const mx_jit_cache_t *bank, uint32_t n, uint32_t reg) {
    for (uint32_t i = 0; i < n; i++) {
        if (bank[i].is_used && bank[i].reg == reg) {
            return (uint8_t)i;
        }
    }
    return MX_JIT_NONE;
}

uint8_t mx_jit_find_g(const mx_jit_cc_t *cc, uint32_t reg) {
    return mx_jit_find(cc->g, 16, cc->off + reg);
}

uint8_t mx_jit_find_x(const mx_jit_cc_t *cc, uint32_t reg) {
    return mx_jit_find(cc->x, MX_JIT_NXMM, cc->off + reg);
}

bool mx_jit_is_cached(const mx_jit_cc_t *cc, uint32_t reg) {
    return mx_jit_find_g(cc, reg) != MX_JIT_NONE || mx_jit_find_x(cc, reg) != MX_JIT_NONE;
}

void mx_jit_set(mx_jit_cc_t *cc, mx_jit_cache_t *c, uint32_t reg, bool is_dirty) {
    cc->age += 1;
    *c = (mx_jit_cache_t) {
        .reg = reg,
        .age = cc->age,
        .is_used = true,
        .is_dirty = is_dirty,
        .is_pinned = true,
    };
}

mx_x64_reg_t mx_jit_alloc_g(mx_jit_cc_t *cc) {
    uint8_t oldest = MX_JIT_NONE;
    for (uint32_t i = 0; i < MX_JIT_NGPR; i++) {
        mx_x64_reg_t r = mx_jit_gprs[i];
        const mx_jit_cache_t *c = &cc->g[r];
        if (!c->is_pinned && !c->is_used) {
            return r;
        } else if (!c->is_pinned && (oldest == MX_JIT_NONE || c->age < cc->g[oldest].age)) {
            oldest = r;
        }
    }
    if (oldest == MX_JIT_NONE) {
        mx_fatal("jit: out of pinned registers");
    }
    mx_jit_cache_t *c = &cc->g[oldest];
    if (c->is_dirty) {
        mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), oldest);
    }
    *c = (mx_jit_cache_t) { .reg = 0 };
    return oldest;
}

uint32_t mx_jit_alloc_x(mx_jit_cc_t *cc) {
    uint8_t oldest = MX_JIT_NONE;
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        const mx_jit_cache_t *c = &cc->x[i];
        if (!c->is_pinned && !c->is_used) {
            return i;
        } else if (!c->is_pinned && (oldest == MX_JIT_NONE || c->age < cc->x[oldest].age)) {
            oldest = (uint8_t)i;
        }
    }
    if (oldest == MX_JIT_NONE) {
        mx_fatal("jit: out of pinned registers");
    }
    mx_jit_cache_t *c = &cc->x[oldest];
    if (c->is_dirty) {
        mx_x64_movsd_store(&cc->hot, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), oldest);
    }
    *c = (mx_jit_cache_t) { .reg = 0 };
    return oldest;
}

void mx_jit_drop(mx_jit_cc_t *cc, uint32_t reg) {
    uint8_t g = mx_jit_find_g(cc, reg);
    if (g != MX_JIT_NONE) {
        cc->g[g].is_used = false;
        cc->g[g].is_dirty = false;
    }
    uint8_t x = mx_jit_find_x(cc, reg);
    if (x != MX_JIT_NONE) {
        cc->x[x].is_used = false;
        cc->x[x].is_dirty = false;
    }
}

void mx_jit_drop_all(mx_jit_cc_t *cc) {
    for (uint32_t i = 0; i < 16; i++) {
        cc->g[i] = (mx_jit_cache_t) { .reg = 0 };
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        cc->x[i] = (mx_jit_cache_t) { .reg = 0 };
    }
}

void mx_jit_drop_caller_saved(mx_jit_cc_t *cc) {
    for (uint32_t i = 0; i < 16; i++) {
        if (mx_jit_is_caller_saved((mx_x64_reg_t)i)) {
            cc->g[i] = (mx_jit_cache_t) { .reg = 0 };
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        cc->x[i] = (mx_jit_cache_t) { .reg = 0 };
    }
}

void mx_jit_unpin(mx_jit_cc_t *cc) {
    for (uint32_t i = 0; i < 16; i++) {
        cc->g[i].is_pinned = false;
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        cc->x[i].is_pinned = false;
    }
}

mx_x64_reg_t mx_jit_use_g(mx_jit_cc_t *cc, uint32_t reg) {
    uint8_t found = mx_jit_find_g(cc, reg);
    if (found != MX_JIT_NONE) {
        cc->age += 1;
        cc->g[found].age = cc->age;
        cc->g[found].is_pinned = true;
        return found;
    }
    uint8_t x = mx_jit_find_x(cc, reg);
    mx_x64_reg_t r = mx_jit_alloc_g(cc);
    if (x != MX_JIT_NONE) {
        mx_x64_movq_rx(&cc->hot, r, x);
        mx_jit_set(cc, &cc->g[r], cc->off + reg, cc->x[x].is_dirty);
        cc->x[x].is_used = false;
        cc->x[x].is_dirty = false;
    } else {
        mx_x64_load(&cc->hot, r, MX_X64_RBX, mx_jit_disp(reg));
        mx_jit_set(cc, &cc->g[r], cc->off + reg, false);
    }
    return r;
}

uint32_t mx_jit_use_x(mx_jit_cc_t *cc, uint32_t reg) {
    uint8_t found = mx_jit_find_x(cc, reg);
    if (found != MX_JIT_NONE) {
        cc->age += 1;
        cc->x[found].age = cc->age;
        cc->x[found].is_pinned = true;
        return found;
    }
    uint8_t g = mx_jit_find_g(cc, reg);
    uint32_t x = mx_jit_alloc_x(cc);
    if (g != MX_JIT_NONE) {
        mx_x64_movq_xr(&cc->hot, x, g);
        mx_jit_set(cc, &cc->x[x], cc->off + reg, cc->g[g].is_dirty);
        cc->g[g].is_used = false;
        cc->g[g].is_dirty = false;
    } else {
        mx_x64_movsd_load(&cc->hot, x, MX_X64_RBX, mx_jit_disp(reg));
        mx_jit_set(cc, &cc->x[x], cc->off + reg, false);
    }
    return x;
}

mx_x64_reg_t mx_jit_def_g(mx_jit_cc_t *cc, uint32_t reg) {
    mx_jit_drop(cc, reg);
    mx_x64_reg_t r = mx_jit_alloc_g(cc);
    mx_jit_set(cc, &cc->g[r], cc->off + reg, true);
    return r;
}

uint32_t mx_jit_def_x(mx_jit_cc_t *cc, uint32_t reg) {
    mx_jit_drop(cc, reg);
    uint32_t x = mx_jit_alloc_x(cc);
    mx_jit_set(cc, &cc->x[x], cc->off + reg, true);
    return x;
}

void mx_jit_spill(const mx_jit_cc_t *cc, mx_x64_t *a) {
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && cc->g[i].is_dirty) {
            mx_x64_store(a, MX_X64_RBX, mx_jit_home_disp(cc, cc->g[i].reg), (mx_x64_reg_t)i);
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used && cc->x[i].is_dirty) {
            mx_x64_movsd_store(a, MX_X64_RBX, mx_jit_home_disp(cc, cc->x[i].reg), i);
        }
    }
}

void mx_jit_flush(mx_jit_cc_t *cc) {
    mx_jit_spill(cc, &cc->hot);
    for (uint32_t i = 0; i < 16; i++) {
        cc->g[i].is_dirty = false;
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        cc->x[i].is_dirty = false;
    }
}

bool mx_jit_keep(const mx_jit_cc_t *cc, uint32_t abs, const uint64_t *live0, const uint64_t *live1) {
    if (abs < cc->off) {
        return true;
    }
    uint32_t reg = abs - cc->off;
    if (reg >= cc->frame) {
        return false;
    } else if (reg >= cc->nregs) {
        return true;
    } else if (reg < cc->rb->nout && mx_kind_is_cell(cc->rb->out[reg])) {
        return true;
    } else {
        return mx_ir_live_test(live0, reg) || mx_ir_live_test(live1, reg);
    }
}

void mx_jit_flush_live(mx_jit_cc_t *cc, mx_x64_t *a, uint32_t skip, const uint64_t *live0, const uint64_t *live1) {
    uint32_t skip_abs = skip;
    if (skip != UINT32_MAX) {
        skip_abs = cc->off + skip;
    }
    for (uint32_t i = 0; i < 16; i++) {
        const mx_jit_cache_t *c = &cc->g[i];
        if (c->is_used && c->is_dirty && c->reg != skip_abs && mx_jit_keep(cc, c->reg, live0, live1)) {
            mx_x64_store(a, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), (mx_x64_reg_t)i);
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        const mx_jit_cache_t *c = &cc->x[i];
        if (c->is_used && c->is_dirty && c->reg != skip_abs && mx_jit_keep(cc, c->reg, live0, live1)) {
            mx_x64_movsd_store(a, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), i);
        }
    }
}

const mx_jit_label_t *mx_jit_label_of(const mx_jit_cc_t *cc, const uintptr_t *code) {
    for (uint32_t i = 0; i < cc->nlabels; i++) {
        const mx_jit_label_t *label = &cc->labels[i];
        if (label->code == code && label->chain == cc->chain) {
            bool is_same = true;
            for (uint32_t h = 0; h < label->nhome && is_same; h++) {
                if (label->homes[h].is_const) {
                    uint8_t g = mx_jit_find(cc->g, 16, label->homes[h].reg);
                    is_same = g != MX_JIT_NONE && cc->g[g].is_const && cc->g[g].bits == label->homes[h].bits;
                }
            }
            if (is_same) {
                return label;
            }
        }
    }
    return NULL;
}

bool mx_jit_can_stitch(const mx_jit_cc_t *cc, const uintptr_t *code) {
    if (code == NULL) {
        return false;
    } else if (code == cc->first && cc->off == 0) {
        return true;
    }
    const mx_version_t *target = (const mx_version_t *)code[-1];
    if (target->is_hooked || target->block->func != cc->rb->block->func || target->block->id <= cc->rb->block->id) {
        return false;
    } else if (mx_jit_label_of(cc, code) != NULL) {
        return true;
    } else if (cc->nlabels + (cc->npend - cc->pend_at) >= MX_JIT_STITCH_MAX || cc->npend >= MX_JIT_STITCH_MAX) {
        return false;
    }
    return code[1] != cc->vm->jit.back || code[2] >= cc->vm->jit.threshold;
}

uint32_t mx_jit_capture(const mx_jit_cc_t *cc, mx_jit_home_t *homes) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used) {
            homes[n] = (mx_jit_home_t) { .bits = cc->g[i].bits, .reg = cc->g[i].reg, .phys = (uint8_t)i, .is_dirty = cc->g[i].is_dirty, .is_const = cc->g[i].is_const };
            n += 1;
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used) {
            homes[n] = (mx_jit_home_t) { .reg = cc->x[i].reg, .phys = (uint8_t)(i + MX_JIT_XMM_ID), .is_dirty = cc->x[i].is_dirty };
            n += 1;
        }
    }
    return n;
}

void mx_jit_restore(mx_jit_cc_t *cc, const mx_jit_home_t *homes, uint32_t n) {
    mx_jit_drop_all(cc);
    for (uint32_t i = 0; i < n; i++) {
        if (homes[i].phys >= MX_JIT_XMM_ID) {
            mx_jit_set(cc, &cc->x[homes[i].phys - MX_JIT_XMM_ID], homes[i].reg, homes[i].is_dirty);
        } else {
            mx_jit_set(cc, &cc->g[homes[i].phys], homes[i].reg, homes[i].is_dirty);
            cc->g[homes[i].phys].is_const = homes[i].is_const;
            cc->g[homes[i].phys].bits = homes[i].bits;
        }
    }
    mx_jit_unpin(cc);
}

void mx_jit_push_pend(mx_jit_cc_t *cc, uintptr_t *code, uint8_t *site) {
    mx_jit_pend_t *p = &cc->pend[cc->npend];
    cc->npend += 1;
    p->code = code;
    p->from = cc->code;
    p->site = site;
    p->rb = cc->rb;
    p->tail = cc->tail;
    p->link_tail = cc->link_tail;
    p->off = cc->off;
    p->nregs = cc->nregs;
    p->frame = cc->frame;
    p->chain = cc->chain;
    p->ninline = cc->ninline;
    p->has_val = false;
    memcpy(p->stack, cc->stack, sizeof(mx_jit_inline_t) * cc->ninline);
    p->nhome = mx_jit_capture(cc, p->homes);
}

void mx_jit_kind_ways(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_kind_t hot, mx_x64_reg_t src, int32_t kind_at, int32_t val_at, uint8_t *stub, uintptr_t flags) {
    uintptr_t keys[MX_RESUME_WAYS];
    uintptr_t *codes[MX_RESUME_WAYS];
    uint32_t n = 0;
    keys[n] = rec->cache.key;
    codes[n] = (uintptr_t *)rec->cache.code;
    n += 1;
    for (uint32_t i = 0; i + 1 < MX_RESUME_WAYS && rec->cache.more != NULL; i++) {
        keys[n] = rec->cache.more[2 * i];
        codes[n] = (uintptr_t *)rec->cache.more[2 * i + 1];
        n += 1;
    }
    uint8_t *miss = cc->cold.p;
    bool has_way = false;
    for (uint32_t i = 0; i < n; i++) {
        mx_kind_t kind = (mx_kind_t)keys[i];
        bool is_value = codes[i] != NULL && mx_jit_key_len(keys[i]) == 2 && (keys[i] & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED)) == flags;
        if (is_value && kind != hot && (kind != MX_KIND_NIL || flags != 0) && mx_jit_can_stitch(cc, codes[i])) {
            mx_x64_cmp16_mi(&cc->cold, src, kind_at, kind);
            uint8_t *other = mx_jit_label(&cc->cold, MX_X64_CC_NE);
            mx_x64_load(&cc->cold, MX_X64_RAX, src, val_at);
            uint8_t *site = mx_jit_label_jmp(&cc->cold);
            mx_jit_push_pend(cc, codes[i], site);
            mx_jit_pend_t *p = &cc->pend[cc->npend - 1];
            p->has_val = true;
            p->val_out = out;
            p->val_kind = kind;
            mx_x64_patch(other, cc->cold.p);
            has_way = true;
        }
    }
    mx_x64_cmp16_mi(&cc->hot, src, kind_at, hot);
    if (has_way) {
        mx_x64_jmp(&cc->cold, stub);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
    } else {
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
}

uint32_t mx_jit_resume(mx_jit_cc_t *cc) {
    const mx_jit_pend_t *p = &cc->pend[cc->pend_at];
    cc->pend_at += 1;
    mx_x64_patch(p->site, cc->hot.p);
    cc->rb = p->rb;
    cc->code = p->from;
    cc->tail = p->tail;
    cc->link_tail = p->link_tail;
    cc->off = p->off;
    cc->nregs = p->nregs;
    cc->frame = p->frame;
    cc->chain = p->chain;
    cc->ninline = p->ninline;
    memcpy(cc->stack, p->stack, sizeof(mx_jit_inline_t) * p->ninline);
    mx_jit_restore(cc, p->homes, p->nhome);
    if (p->has_val && p->val_kind == MX_KIND_FLOAT) {
        uint32_t x = mx_jit_def_x(cc, p->val_out);
        mx_x64_movq_xr(&cc->hot, x, MX_X64_RAX);
    } else if (p->has_val) {
        mx_x64_reg_t d = mx_jit_def_g(cc, p->val_out);
        mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
    }
    return mx_jit_continue(cc, p->code);
}

void mx_jit_tails(mx_jit_cc_t *cc) {
    cc->tail = NULL;
    cc->link_tail = NULL;
}

void mx_jit_tail_code(mx_jit_cc_t *cc, mx_x64_t *a) {
    cc->tail = a->p;
    mx_x64_mov_ri(a, MX_X64_RDX, (uint64_t)(uintptr_t)cc->rb);
    mx_x64_jmp(a, cc->vm->jit.exit);
}

uint32_t mx_jit_continue(mx_jit_cc_t *cc, uintptr_t *code) {
    const mx_jit_label_t *label = mx_jit_label_of(cc, code);
    if (code == cc->first && cc->off == 0) {
        mx_jit_back_edge(cc);
        return 0;
    } else if (label != NULL) {
        mx_jit_merge(cc, label->homes, label->nhome, mx_jit_live(cc, ((mx_version_t *)code[-1])->block));
        mx_x64_jmp(&cc->hot, label->at);
        return 0;
    } else {
        return mx_jit_stitch(cc, code);
    }
}

void mx_jit_snapshot(mx_jit_cc_t *cc) {
    const mx_version_t *first = (const mx_version_t *)cc->first[-1];
    const uint64_t *live = mx_jit_live(cc, first->block);
    for (uint32_t i = 0; i < MX_JIT_NPHYS; i++) {
        const mx_jit_cache_t *c;
        if (i < 16) {
            c = &cc->g[i];
        } else {
            c = &cc->x[i - 16];
        }
        bool is_cell = (c->reg < cc->rb->nout && mx_kind_is_cell(cc->rb->out[c->reg])) || (c->reg < first->nkinds && mx_kind_is_cell(first->kinds[c->reg]));
        if (c->is_used && c->reg < cc->nregs && !is_cell && mx_ir_live_test(live, c->reg)) {
            bool is_placed = false;
            for (uint32_t s = 0; s < cc->nsnap; s++) {
                if (cc->snap[s].reg == c->reg) {
                    cc->snap[s].is_dirty = cc->snap[s].is_dirty || c->is_dirty;
                    is_placed = true;
                } else if (cc->snap[s].phys == i) {
                    is_placed = true;
                }
            }
            if (!is_placed) {
                cc->snap[cc->nsnap] = (mx_jit_home_t) { .reg = c->reg, .phys = (uint8_t)i, .is_dirty = c->is_dirty };
                cc->nsnap += 1;
            }
        }
    }
}

void mx_jit_move_phys(mx_jit_cc_t *cc, uint8_t dst, uint8_t src) {
    bool dst_x = dst >= MX_JIT_XMM_ID;
    bool src_x = src >= MX_JIT_XMM_ID;
    if (dst_x && src_x) {
        mx_x64_movaps(&cc->hot, (uint32_t)dst - MX_JIT_XMM_ID, (uint32_t)src - MX_JIT_XMM_ID);
    } else if (dst_x) {
        mx_x64_movq_xr(&cc->hot, (uint32_t)dst - MX_JIT_XMM_ID, src);
    } else if (src_x) {
        mx_x64_movq_rx(&cc->hot, dst, (uint32_t)src - MX_JIT_XMM_ID);
    } else {
        mx_x64_mov_rr(&cc->hot, dst, src);
    }
}

void mx_jit_parallel_moves(mx_jit_cc_t *cc, mx_jit_move_t *moves, uint32_t n) {
    uint32_t left = n;
    while (left != 0) {
        bool is_done = false;
        for (uint32_t i = 0; i < left && !is_done; i++) {
            bool is_blocked = false;
            for (uint32_t j = 0; j < left; j++) {
                is_blocked = is_blocked || (j != i && moves[j].src == moves[i].dst);
            }
            if (!is_blocked) {
                mx_jit_move_phys(cc, moves[i].dst, moves[i].src);
                moves[i] = moves[left - 1];
                left -= 1;
                is_done = true;
            }
        }
        if (!is_done) {
            uint8_t held = moves[0].dst;
            uint8_t scratch;
            if (held >= MX_JIT_XMM_ID) {
                scratch = MX_JIT_XMM_ID + MX_JIT_XMM_SCRATCH;
            } else {
                scratch = MX_X64_RAX;
            }
            mx_jit_move_phys(cc, scratch, held);
            for (uint32_t j = 0; j < left; j++) {
                if (moves[j].src == held) {
                    moves[j].src = scratch;
                }
            }
        }
    }
}

void mx_jit_merge(mx_jit_cc_t *cc, const mx_jit_home_t *homes, uint32_t n, const uint64_t *live) {
    for (uint32_t i = 0; i < MX_JIT_NPHYS; i++) {
        mx_jit_cache_t *c;
        if (i < 16) {
            c = &cc->g[i];
        } else {
            c = &cc->x[i - 16];
        }
        bool is_carried = false;
        for (uint32_t h = 0; h < n; h++) {
            is_carried = is_carried || (homes[h].reg == c->reg && homes[h].is_dirty);
        }
        if (c->is_used && c->is_dirty && !is_carried && mx_jit_keep(cc, c->reg, live, live)) {
            if (i < 16) {
                mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), (mx_x64_reg_t)i);
            } else {
                mx_x64_movsd_store(&cc->hot, MX_X64_RBX, mx_jit_home_disp(cc, c->reg), i - 16);
            }
        }
    }
    mx_jit_move_t moves[MX_JIT_NHOME];
    uint32_t nmoves = 0;
    for (uint32_t h = 0; h < n; h++) {
        const mx_jit_home_t *home = &homes[h];
        uint8_t g = mx_jit_find(cc->g, 16, home->reg);
        uint8_t x = mx_jit_find(cc->x, MX_JIT_NXMM, home->reg);
        uint8_t src;
        if (g != MX_JIT_NONE) {
            src = g;
        } else if (x != MX_JIT_NONE) {
            src = (uint8_t)(x + MX_JIT_XMM_ID);
        } else {
            src = MX_JIT_NONE;
        }
        if (src != MX_JIT_NONE && src != home->phys) {
            moves[nmoves] = (mx_jit_move_t) { .src = src, .dst = home->phys };
            nmoves += 1;
        }
    }
    mx_jit_parallel_moves(cc, moves, nmoves);
    for (uint32_t h = 0; h < n; h++) {
        const mx_jit_home_t *home = &homes[h];
        if (mx_jit_find(cc->g, 16, home->reg) == MX_JIT_NONE && mx_jit_find(cc->x, MX_JIT_NXMM, home->reg) == MX_JIT_NONE) {
            if (home->phys >= MX_JIT_XMM_ID) {
                mx_x64_movsd_load(&cc->hot, (uint32_t)home->phys - MX_JIT_XMM_ID, MX_X64_RBX, mx_jit_home_disp(cc, home->reg));
            } else {
                mx_x64_load(&cc->hot, home->phys, MX_X64_RBX, mx_jit_home_disp(cc, home->reg));
            }
        }
    }
}

void mx_jit_back_edge(mx_jit_cc_t *cc) {
    mx_jit_snapshot(cc);
    const mx_version_t *first = (const mx_version_t *)cc->first[-1];
    mx_jit_merge(cc, cc->head, cc->nhead, mx_jit_live(cc, first->block));
    uint8_t *poll = cc->cold.p;
    for (uint32_t h = 0; h < cc->nhead; h++) {
        const mx_jit_home_t *home = &cc->head[h];
        if (home->is_dirty && home->phys >= MX_JIT_XMM_ID) {
            mx_x64_movsd_store(&cc->cold, MX_X64_RBX, mx_jit_disp(home->reg), (uint32_t)home->phys - MX_JIT_XMM_ID);
        } else if (home->is_dirty) {
            mx_x64_store(&cc->cold, MX_X64_RBX, mx_jit_disp(home->reg), home->phys);
        }
    }
    mx_x64_mov_ri(&cc->cold, MX_X64_RAX, 0);
    mx_x64_jmp(&cc->cold, cc->first_tail);
    mx_x64_cmp8_mi(&cc->hot, MX_X64_R12, (int32_t)offsetof(mx_t, interrupt), 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, poll);
    mx_x64_jmp(&cc->hot, cc->head_label);
}

uint32_t mx_jit_stitch(mx_jit_cc_t *cc, uintptr_t *code) {
    mx_version_t *target = (mx_version_t *)code[-1];
    const uint64_t *live = mx_jit_live(cc, target->block);
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && !mx_jit_keep(cc, cc->g[i].reg, live, live)) {
            cc->g[i].is_used = false;
            cc->g[i].is_dirty = false;
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used && !mx_jit_keep(cc, cc->x[i].reg, live, live)) {
            cc->x[i].is_used = false;
            cc->x[i].is_dirty = false;
        }
    }
    cc->rb = target;
    cc->code = code;
    cc->ip = code;
    mx_jit_label_t *label = &cc->labels[cc->nlabels];
    cc->nlabels += 1;
    label->code = code;
    label->at = cc->hot.p;
    label->chain = cc->chain;
    label->nhome = mx_jit_capture(cc, label->homes);
    mx_jit_tails(cc);
    return MX_JIT_ENTRY_WORDS;
}

const uint64_t *mx_jit_live(mx_jit_cc_t *cc, mx_ir_block_t *block) {
    if (block->func->live == NULL) {
        mx_ir_liveness(cc->vm, mx_ir_func_head(block));
    }
    return mx_ir_block_live(block);
}

void mx_jit_exit_to(mx_jit_cc_t *cc, mx_x64_t *a) {
    mx_x64_mov_ri(a, MX_X64_RAX, (uint64_t)(cc->ip - cc->code));
    if (cc->tail != NULL) {
        mx_x64_jmp(a, cc->tail);
    } else {
        mx_jit_tail_code(cc, a);
    }
}

uint8_t *mx_jit_exit_stub(mx_jit_cc_t *cc) {
    if (cc->hop_exit == NULL) {
        cc->hop_exit = cc->cold.p;
        mx_jit_spill(cc, &cc->cold);
        mx_jit_exit_to(cc, &cc->cold);
    }
    return cc->hop_exit;
}

void mx_jit_stop(mx_jit_cc_t *cc) {
    mx_x64_jmp(&cc->hot, mx_jit_exit_stub(cc));
}

void mx_jit_call_rt(mx_jit_cc_t *cc, mx_x64_t *a, mx_jit_slot_t slot, mx_hop_t hop) {
    mx_x64_mov_rr(a, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_rr(a, MX_X64_RSI, MX_X64_RBX);
    mx_x64_mov_ri(a, MX_X64_RDX, (uint64_t)(uintptr_t)cc->ip);
    mx_x64_mov_ri(a, MX_X64_RCX, hop);
    mx_x64_call_rip(a, &cc->vm->jit.slots[slot]);
}

uint8_t *mx_jit_slow_stub(mx_jit_cc_t *cc, mx_hop_t hop) {
    uint8_t *stub = cc->cold.p;
    mx_jit_spill(cc, &cc->cold);
    if (hop == MX_HOP_TABLE_INIT_SK || hop == MX_HOP_TABLE_INIT_IK || hop == MX_HOP_UPVAL_SET) {
        mx_jit_call_rt(cc, &cc->cold, MX_JIT_SLOT_HOP, hop);
    } else {
        mx_jit_call_rt(cc, &cc->cold, MX_JIT_SLOT_SET, hop);
        mx_x64_test8_al(&cc->cold);
        uint8_t *ok = mx_jit_label(&cc->cold, MX_X64_CC_NE);
        mx_jit_exit_to(cc, &cc->cold);
        mx_x64_patch(ok, cc->cold.p);
    }
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && mx_jit_is_caller_saved((mx_x64_reg_t)i)) {
            mx_x64_load(&cc->cold, (mx_x64_reg_t)i, MX_X64_RBX, mx_jit_home_disp(cc, cc->g[i].reg));
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used) {
            mx_x64_movsd_load(&cc->cold, i, MX_X64_RBX, mx_jit_home_disp(cc, cc->x[i].reg));
        }
    }
    cc->slow_join = mx_jit_label_jmp(&cc->cold);
    return stub;
}

void mx_jit_slow_join(mx_jit_cc_t *cc) {
    mx_x64_patch(cc->slow_join, cc->hot.p);
}

mx_jit_slot_t mx_jit_get_slot(mx_hop_t hop) {
    switch (hop) {
        case MX_HOP_TABLE_GET_SK: {
            return MX_JIT_SLOT_GET_SK;
        }
        case MX_HOP_TABLE_GET_IR: {
            return MX_JIT_SLOT_GET_IR;
        }
        case MX_HOP_TABLE_GET_IK: {
            return MX_JIT_SLOT_GET_IK;
        }
        case MX_HOP_METHOD_GET_SK: {
            return MX_JIT_SLOT_GET_METHOD;
        }
        case MX_HOP_UPVAL_GET: {
            return MX_JIT_SLOT_GET_UPVAL;
        }
        default: {
            return MX_JIT_SLOT_GET_VAR;
        }
    }
}

mx_jit_slot_t mx_jit_hop_slot(mx_hop_t hop) {
    switch (hop) {
        case MX_HOP_TABLE_NEW: {
            return MX_JIT_SLOT_TABLE_NEW;
        }
        case MX_HOP_CONCAT: {
            return MX_JIT_SLOT_CONCAT;
        }
        case MX_HOP_CLOSURE: {
            return MX_JIT_SLOT_CLOSURE;
        }
        case MX_HOP_CLOSE: {
            return MX_JIT_SLOT_CLOSE;
        }
        default: {
            return MX_JIT_SLOT_HOP;
        }
    }
}

uint8_t *mx_jit_get_stub(mx_jit_cc_t *cc, mx_hop_t hop) {
    uint8_t *stub = cc->cold.p;
    mx_jit_spill(cc, &cc->cold);
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(uintptr_t)cc->ip);
    mx_x64_mov_ri(&cc->cold, MX_X64_RCX, (uint64_t)(uintptr_t)cc->first[-1]);
    mx_x64_mov_ri(&cc->cold, MX_X64_R8, (uint64_t)(uintptr_t)cc->rb);
    mx_x64_jmp(&cc->cold, cc->vm->jit.slots[MX_JIT_SLOT_T_GET_SK + (mx_jit_get_slot(hop) - MX_JIT_SLOT_GET_SK)]);
    return stub;
}

void mx_jit_hop_call(mx_jit_cc_t *cc, mx_hop_t hop) {
    mx_jit_flush(cc);
    mx_jit_call_rt(cc, &cc->hot, mx_jit_hop_slot(hop), hop);
    mx_jit_drop_caller_saved(cc);
}

uint8_t *mx_jit_entry_of(const mx_jit_cc_t *cc, const uintptr_t *code, uint32_t info) {
    if (code == NULL) {
        return NULL;
    } else if (code == cc->first) {
        return cc->entry;
    } else if (code[1] == cc->vm->jit.back) {
        return NULL;
    }
    const mx_version_t *target = (const mx_version_t *)code[-1];
    if ((info & 1) == 0 && target->block->id > cc->rb->block->id) {
        return (uint8_t *)code[1] + cc->vm->jit.poll_len;
    } else {
        return (uint8_t *)code[1];
    }
}

void mx_jit_edge_stub(mx_jit_cc_t *cc, uint8_t *site, uintptr_t *word, uint32_t info) {
    mx_x64_patch(site, cc->cold.p);
    mx_x64_mov_ri(&cc->cold, MX_X64_RSI, (uint64_t)(uintptr_t)word);
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(site - cc->vm->jit.mem));
    mx_x64_mov_ri(&cc->cold, MX_X64_RCX, info);
    if (cc->link_tail != NULL) {
        mx_x64_jmp(&cc->cold, cc->link_tail);
    } else {
        cc->link_tail = cc->cold.p;
        mx_x64_mov_ri(&cc->cold, MX_X64_R8, (uint64_t)(uintptr_t)cc->rb);
        mx_x64_mov_ri(&cc->cold, MX_X64_R9, (uint64_t)(uintptr_t)cc->first[-1]);
        mx_x64_jmp(&cc->cold, cc->vm->jit.link);
    }
}

void mx_jit_edge(mx_jit_cc_t *cc, mx_x64_cc_t cond, uintptr_t *word, uint32_t info) {
    uint8_t *to = mx_jit_entry_of(cc, (const uintptr_t *)*word, info);
    if (to != NULL) {
        mx_x64_jcc(&cc->hot, cond, to);
    } else {
        uint8_t *site = mx_jit_label(&cc->hot, cond);
        mx_jit_edge_stub(cc, site, word, info);
    }
}

void mx_jit_edge_jmp(mx_jit_cc_t *cc, uintptr_t *word, uint32_t info) {
    uint8_t *to = mx_jit_entry_of(cc, (const uintptr_t *)*word, info);
    if (to != NULL) {
        mx_x64_jmp(&cc->hot, to);
    } else {
        uint8_t *site = mx_jit_label_jmp(&cc->hot);
        mx_jit_edge_stub(cc, site, word, info);
    }
}

uint8_t *mx_jit_label(mx_x64_t *a, mx_x64_cc_t cond) {
    mx_x64_jcc(a, cond, a->p);
    return a->p;
}

uint8_t *mx_jit_label_jmp(mx_x64_t *a) {
    mx_x64_jmp(a, a->p);
    return a->p;
}

bool mx_jit_fits32(uint64_t imm) {
    return (uint64_t)(int64_t)(int32_t)imm == imm;
}

uintptr_t *mx_jit_cell(mx_jit_cc_t *cc, const uintptr_t *code) {
    while (((uintptr_t)cc->cold.p & (sizeof(uintptr_t) - 1)) != 0) {
        mx_x64_u8(&cc->cold, 0xcc);
    }
    uintptr_t *cell = (uintptr_t *)cc->cold.p;
    mx_x64_u64(&cc->cold, (uint64_t)(uintptr_t)code);
    return cell;
}

const uint8_t *mx_jit_pool(mx_jit_cc_t *cc, uint64_t bits) {
    const uint8_t *at = cc->cold.p;
    mx_x64_u64(&cc->cold, bits);
    return at;
}

void mx_jit_load_lit_x(mx_jit_cc_t *cc, uint32_t x, uint64_t bits) {
    if (bits == 0) {
        mx_x64_xorpd(&cc->hot, x, x);
    } else {
        mx_x64_movsd_rip(&cc->hot, x, mx_jit_pool(cc, bits));
    }
}

void mx_jit_load_reg(mx_jit_cc_t *cc, mx_x64_reg_t d, uint32_t reg) {
    uint8_t g = mx_jit_find_g(cc, reg);
    uint8_t x = mx_jit_find_x(cc, reg);
    if (g != MX_JIT_NONE) {
        mx_x64_mov_rr(&cc->hot, d, g);
    } else if (x != MX_JIT_NONE) {
        mx_x64_movq_rx(&cc->hot, d, x);
    } else {
        mx_x64_load(&cc->hot, d, MX_X64_RBX, mx_jit_disp(reg));
    }
}

void mx_jit_load_arg(mx_jit_cc_t *cc, mx_x64_reg_t d, const uintptr_t *arg) {
    if ((arg[1] & 1) != 0) {
        mx_x64_mov_ri(&cc->hot, d, arg[0]);
    } else {
        mx_jit_load_reg(cc, d, (uint32_t)arg[0]);
    }
}

void mx_jit_store_arg(mx_jit_cc_t *cc, mx_x64_reg_t base, int32_t disp, const uintptr_t *arg, mx_x64_reg_t scratch) {
    if ((arg[1] & 1) != 0) {
        if (mx_jit_fits32(arg[0])) {
            mx_x64_store_i32(&cc->hot, base, disp, (int32_t)arg[0]);
        } else {
            mx_x64_mov_ri(&cc->hot, scratch, arg[0]);
            mx_x64_store(&cc->hot, base, disp, scratch);
        }
        return;
    }
    uint32_t reg = (uint32_t)arg[0];
    uint8_t g = mx_jit_find_g(cc, reg);
    uint8_t x = mx_jit_find_x(cc, reg);
    if (g != MX_JIT_NONE) {
        mx_x64_store(&cc->hot, base, disp, g);
    } else if (x != MX_JIT_NONE) {
        mx_x64_movsd_store(&cc->hot, base, disp, x);
    } else {
        mx_x64_load(&cc->hot, scratch, MX_X64_RBX, mx_jit_disp(reg));
        mx_x64_store(&cc->hot, base, disp, scratch);
    }
}

void mx_jit_move_r(mx_jit_cc_t *cc, uint32_t out, uint32_t a) {
    if (mx_jit_find_x(cc, a) != MX_JIT_NONE) {
        uint32_t s = mx_jit_use_x(cc, a);
        uint32_t d = mx_jit_def_x(cc, out);
        mx_x64_movaps(&cc->hot, d, s);
    } else {
        mx_x64_reg_t s = mx_jit_use_g(cc, a);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_mov_rr(&cc->hot, d, s);
        cc->g[d].is_const = cc->g[s].is_const;
        cc->g[d].bits = cc->g[s].bits;
    }
}

void mx_jit_move_i(mx_jit_cc_t *cc, uint32_t out, uint64_t bits) {
    mx_x64_reg_t d = mx_jit_def_g(cc, out);
    mx_x64_mov_ri(&cc->hot, d, bits);
    cc->g[d].is_const = true;
    cc->g[d].bits = bits;
}

const mx_jit_cache_t *mx_jit_const_of(const mx_jit_cc_t *cc, uint32_t reg) {
    uint8_t g = mx_jit_find_g(cc, reg);
    if (g != MX_JIT_NONE && cc->g[g].is_const) {
        return &cc->g[g];
    }
    return NULL;
}

mx_x64_reg_t mx_jit_target_g(mx_jit_cc_t *cc, uint32_t out, uint32_t a, mx_x64_reg_t ra) {
    if (out == a) {
        cc->g[ra].is_dirty = true;
        cc->g[ra].is_const = false;
        return ra;
    }
    mx_x64_reg_t d = mx_jit_def_g(cc, out);
    mx_x64_mov_rr(&cc->hot, d, ra);
    return d;
}

uint32_t mx_jit_target_x(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint32_t ra) {
    if (out == a) {
        cc->x[ra].is_dirty = true;
        return ra;
    }
    uint32_t d = mx_jit_def_x(cc, out);
    mx_x64_movaps(&cc->hot, d, ra);
    return d;
}

void mx_jit_alu(mx_jit_cc_t *cc, mx_x64_alu_t op, uint32_t out, uint32_t a, uint32_t b) {
    if (out == b && out != a && op != MX_X64_ALU_SUB) {
        mx_jit_alu(cc, op, out, b, a);
        return;
    }
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    if (mx_jit_is_cached(cc, b)) {
        mx_x64_reg_t rb = mx_jit_use_g(cc, b);
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_alu_rr(&cc->hot, op, d, rb);
    } else {
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_alu_rm(&cc->hot, op, d, MX_X64_RBX, mx_jit_disp(b));
    }
}

void mx_jit_alu_i(mx_jit_cc_t *cc, mx_x64_alu_t op, uint32_t out, uint32_t a, uint64_t imm) {
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    if (op == MX_X64_ALU_ADD && out != a && mx_jit_fits32(imm)) {
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_lea(&cc->hot, d, ra, (int32_t)imm);
    } else if (mx_jit_fits32(imm)) {
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_alu_ri(&cc->hot, op, d, (int32_t)imm);
    } else {
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, imm);
        mx_x64_alu_rr(&cc->hot, op, d, MX_X64_RAX);
    }
}

void mx_jit_sub_ir(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint64_t imm) {
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, imm);
    mx_x64_alu_rr(&cc->hot, MX_X64_ALU_SUB, MX_X64_RAX, ra);
    mx_x64_reg_t d = mx_jit_def_g(cc, out);
    mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
}

void mx_jit_mul(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint32_t b) {
    if (out == b && out != a) {
        mx_jit_mul(cc, out, b, a);
        return;
    }
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    if (mx_jit_is_cached(cc, b)) {
        mx_x64_reg_t rb = mx_jit_use_g(cc, b);
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_imul_rr(&cc->hot, d, rb);
    } else {
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_imul_rm(&cc->hot, d, MX_X64_RBX, mx_jit_disp(b));
    }
}

void mx_jit_mul_i(mx_jit_cc_t *cc, uint32_t out, uint32_t a, uint64_t imm) {
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    if (mx_jit_fits32(imm)) {
        if (out == a) {
            cc->g[ra].is_dirty = true;
            cc->g[ra].is_const = false;
            mx_x64_imul_rri(&cc->hot, ra, ra, (int32_t)imm);
        } else {
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_imul_rri(&cc->hot, d, ra, (int32_t)imm);
        }
    } else {
        mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, imm);
        mx_x64_imul_rr(&cc->hot, d, MX_X64_RAX);
    }
}

void mx_jit_load_opnd(mx_jit_cc_t *cc, mx_x64_reg_t d, mx_jit_opnd_t o) {
    if (o.is_imm) {
        mx_x64_mov_ri(&cc->hot, d, o.value);
    } else {
        mx_jit_load_reg(cc, d, (uint32_t)o.value);
    }
}

void mx_jit_divmod(mx_jit_cc_t *cc, bool is_mod, uint32_t out, mx_jit_opnd_t a, mx_jit_opnd_t d) {
    if (d.is_imm && d.value == 0) {
        mx_jit_stop(cc);
        return;
    }
    mx_jit_load_opnd(cc, MX_X64_RCX, d);
    if (!d.is_imm) {
        mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_E, mx_jit_exit_stub(cc));
    }
    uint8_t *neg = NULL;
    if (!d.is_imm) {
        mx_x64_alu_ri(&cc->hot, MX_X64_ALU_CMP, MX_X64_RCX, -1);
        neg = mx_jit_label(&cc->hot, MX_X64_CC_E);
    }
    uint8_t *done = NULL;
    if (!d.is_imm || d.value != UINT64_MAX) {
        mx_jit_load_opnd(cc, MX_X64_RAX, a);
        mx_x64_cqo(&cc->hot);
        mx_x64_idiv(&cc->hot, MX_X64_RCX);
        mx_x64_test_rr(&cc->hot, MX_X64_RDX, MX_X64_RDX);
        uint8_t *exact = mx_jit_label(&cc->hot, MX_X64_CC_E);
        if (is_mod) {
            mx_x64_mov_rr(&cc->hot, MX_X64_RAX, MX_X64_RDX);
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_XOR, MX_X64_RAX, MX_X64_RCX);
            uint8_t *same = mx_jit_label(&cc->hot, MX_X64_CC_GE);
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_ADD, MX_X64_RDX, MX_X64_RCX);
            mx_x64_patch(same, cc->hot.p);
            mx_x64_patch(exact, cc->hot.p);
            mx_x64_mov_rr(&cc->hot, MX_X64_RAX, MX_X64_RDX);
        } else {
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_XOR, MX_X64_RDX, MX_X64_RCX);
            uint8_t *same = mx_jit_label(&cc->hot, MX_X64_CC_GE);
            mx_x64_alu_ri(&cc->hot, MX_X64_ALU_SUB, MX_X64_RAX, 1);
            mx_x64_patch(same, cc->hot.p);
            mx_x64_patch(exact, cc->hot.p);
        }
        if (!d.is_imm) {
            done = mx_jit_label_jmp(&cc->hot);
        }
    }
    if (neg != NULL) {
        mx_x64_patch(neg, cc->hot.p);
    }
    if (!d.is_imm || d.value == UINT64_MAX) {
        if (is_mod) {
            mx_x64_mov_ri(&cc->hot, MX_X64_RAX, 0);
        } else {
            mx_jit_load_opnd(cc, MX_X64_RAX, a);
            mx_x64_neg(&cc->hot, MX_X64_RAX);
        }
    }
    if (done != NULL) {
        mx_x64_patch(done, cc->hot.p);
    }
    mx_x64_reg_t r = mx_jit_def_g(cc, out);
    mx_x64_mov_rr(&cc->hot, r, MX_X64_RAX);
}

bool mx_jit_divmod_pow2(mx_jit_cc_t *cc, bool is_mod, uint32_t out, uint32_t a, int64_t d) {
    if (d <= 0 || (d & (d - 1)) != 0) {
        return false;
    }
    if (is_mod) {
        mx_jit_alu_i(cc, MX_X64_ALU_AND, out, a, (uint64_t)(d - 1));
    } else {
        mx_x64_reg_t ra = mx_jit_use_g(cc, a);
        mx_x64_reg_t r = mx_jit_target_g(cc, out, a, ra);
        if (d != 1) {
            mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SAR, r, (uint8_t)__builtin_ctzll((uint64_t)d));
        }
    }
    return true;
}

void mx_jit_shift_i(mx_jit_cc_t *cc, bool is_left, uint32_t out, uint32_t a, int64_t count) {
    if (count <= -64 || count >= 64) {
        mx_jit_move_i(cc, out, 0);
        return;
    }
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    mx_x64_reg_t r = mx_jit_target_g(cc, out, a, ra);
    if (count == 0) {
        return;
    } else if ((count > 0) == is_left) {
        uint8_t by;
        if (count < 0) {
            by = (uint8_t)-count;
        } else {
            by = (uint8_t)count;
        }
        mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, r, by);
    } else {
        uint8_t by;
        if (count < 0) {
            by = (uint8_t)-count;
        } else {
            by = (uint8_t)count;
        }
        mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHR, r, by);
    }
}

void mx_jit_sse(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint32_t b) {
    if (out == b && out != a && (op == MX_X64_SSE_ADD || op == MX_X64_SSE_MUL)) {
        mx_jit_sse(cc, op, out, b, a);
        return;
    }
    uint32_t ra = mx_jit_use_x(cc, a);
    if (mx_jit_is_cached(cc, b)) {
        uint32_t rb = mx_jit_use_x(cc, b);
        uint32_t d = mx_jit_target_x(cc, out, a, ra);
        mx_x64_sse_rr(&cc->hot, op, d, rb);
    } else {
        uint32_t d = mx_jit_target_x(cc, out, a, ra);
        mx_x64_sse_rm(&cc->hot, op, d, MX_X64_RBX, mx_jit_disp(b));
    }
}

void mx_jit_sse_ri(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint64_t imm) {
    uint32_t ra = mx_jit_use_x(cc, a);
    uint32_t d = mx_jit_target_x(cc, out, a, ra);
    mx_x64_sse_rip(&cc->hot, op, d, mx_jit_pool(cc, imm));
}

void mx_jit_sse_ir(mx_jit_cc_t *cc, mx_x64_sse_t op, uint32_t out, uint32_t a, uint64_t imm) {
    uint32_t ra = mx_jit_use_x(cc, a);
    mx_jit_load_lit_x(cc, MX_JIT_XMM_SCRATCH, imm);
    mx_x64_sse_rr(&cc->hot, op, MX_JIT_XMM_SCRATCH, ra);
    uint32_t d = mx_jit_def_x(cc, out);
    mx_x64_movaps(&cc->hot, d, MX_JIT_XMM_SCRATCH);
}

void mx_jit_to_f64(mx_jit_cc_t *cc, uint32_t x, uint32_t reg) {
    mx_x64_xorpd(&cc->hot, x, x);
    uint8_t g = mx_jit_find_g(cc, reg);
    uint8_t cached_x = mx_jit_find_x(cc, reg);
    if (g != MX_JIT_NONE) {
        mx_x64_cvtsi2sd_rr(&cc->hot, x, g);
    } else if (cached_x != MX_JIT_NONE) {
        mx_x64_movq_rx(&cc->hot, MX_X64_RAX, cached_x);
        mx_x64_cvtsi2sd_rr(&cc->hot, x, MX_X64_RAX);
    } else {
        mx_x64_cvtsi2sd_rm(&cc->hot, x, MX_X64_RBX, mx_jit_disp(reg));
    }
}

void mx_jit_neg_f64(mx_jit_cc_t *cc, uint32_t out, uint32_t a) {
    mx_jit_load_reg(cc, MX_X64_RAX, a);
    mx_x64_btc_i(&cc->hot, MX_X64_RAX, 63);
    uint32_t d = mx_jit_def_x(cc, out);
    mx_x64_movq_xr(&cc->hot, d, MX_X64_RAX);
}

void mx_jit_unary_i(mx_jit_cc_t *cc, bool is_neg, uint32_t out, uint32_t a) {
    mx_x64_reg_t ra = mx_jit_use_g(cc, a);
    mx_x64_reg_t d = mx_jit_target_g(cc, out, a, ra);
    if (is_neg) {
        mx_x64_neg(&cc->hot, d);
    } else {
        mx_x64_not(&cc->hot, d);
    }
}

mx_jit_plan_t mx_jit_plan(const mx_jit_cc_t *cc) {
    const uintptr_t *taken = (const uintptr_t *)cc->ip[3];
    const uintptr_t *other = (const uintptr_t *)cc->ip[4];
    bool can_taken = mx_jit_can_stitch(cc, taken);
    bool can_other = mx_jit_can_stitch(cc, other);
    if (can_taken && can_other) {
        if (other[2] > taken[2]) {
            return MX_JIT_PLAN_OTHER;
        } else {
            return MX_JIT_PLAN_TAKEN;
        }
    } else if (can_taken) {
        return MX_JIT_PLAN_TAKEN;
    } else if (can_other) {
        return MX_JIT_PLAN_OTHER;
    } else {
        return MX_JIT_PLAN_NONE;
    }
}

void mx_jit_flush_plan(mx_jit_cc_t *cc, mx_jit_plan_t plan) {
    if (plan == MX_JIT_PLAN_NONE) {
        const uint64_t *live0 = mx_jit_live(cc, (mx_ir_block_t *)cc->ip[1]);
        const uint64_t *live1 = mx_jit_live(cc, (mx_ir_block_t *)cc->ip[2]);
        mx_jit_flush_live(cc, &cc->hot, UINT32_MAX, live0, live1);
    }
}

uint8_t *mx_jit_side(mx_jit_cc_t *cc, uintptr_t *word, mx_ir_block_t *block) {
    uint8_t *side = cc->cold.p;
    const uint64_t *live = mx_jit_live(cc, block);
    mx_jit_flush_live(cc, &cc->cold, UINT32_MAX, live, live);
    uint8_t *site = mx_jit_label_jmp(&cc->cold);
    mx_jit_edge_stub(cc, site, word, (2 << 1) | MX_JIT_EDGE_SIDE);
    return side;
}

uint32_t mx_jit_edges(mx_jit_cc_t *cc, mx_jit_plan_t plan, mx_x64_cc_t taken) {
    if (plan == MX_JIT_PLAN_TAKEN) {
        uintptr_t *code = (uintptr_t *)cc->ip[4];
        if (mx_jit_can_stitch(cc, code)) {
            uint8_t *site = mx_jit_label(&cc->hot, taken ^ 1);
            mx_jit_push_pend(cc, code, site);
        } else {
            uint8_t *side = mx_jit_side(cc, &cc->ip[4], (mx_ir_block_t *)cc->ip[2]);
            mx_x64_jcc(&cc->hot, taken ^ 1, side);
        }
        return mx_jit_continue(cc, (uintptr_t *)cc->ip[3]);
    } else if (plan == MX_JIT_PLAN_OTHER) {
        uintptr_t *code = (uintptr_t *)cc->ip[3];
        if (mx_jit_can_stitch(cc, code)) {
            uint8_t *site = mx_jit_label(&cc->hot, taken);
            mx_jit_push_pend(cc, code, site);
        } else {
            uint8_t *side = mx_jit_side(cc, &cc->ip[3], (mx_ir_block_t *)cc->ip[1]);
            mx_x64_jcc(&cc->hot, taken, side);
        }
        return mx_jit_continue(cc, (uintptr_t *)cc->ip[4]);
    }
    mx_jit_edge(cc, taken, &cc->ip[3], 2 << 1);
    mx_jit_edge_jmp(cc, &cc->ip[4], 2 << 1);
    return 0;
}

uint32_t mx_jit_br_i(mx_jit_cc_t *cc, mx_x64_cc_t rel, mx_x64_cc_t swapped, mx_jit_form_t form) {
    mx_jit_plan_t plan = mx_jit_plan(cc);
    mx_x64_reg_t ra = mx_jit_use_g(cc, (uint32_t)cc->ip[5]);
    if (form == MX_JIT_FORM_RR) {
        uint32_t b = (uint32_t)cc->ip[6];
        if (mx_jit_is_cached(cc, b)) {
            mx_x64_reg_t rb = mx_jit_use_g(cc, b);
            mx_jit_flush_plan(cc, plan);
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, ra, rb);
        } else {
            mx_jit_flush_plan(cc, plan);
            mx_x64_alu_rm(&cc->hot, MX_X64_ALU_CMP, ra, MX_X64_RBX, mx_jit_disp(b));
        }
    } else {
        mx_jit_flush_plan(cc, plan);
        if (mx_jit_fits32(cc->ip[6])) {
            mx_x64_alu_ri(&cc->hot, MX_X64_ALU_CMP, ra, (int32_t)cc->ip[6]);
        } else {
            mx_x64_mov_ri(&cc->hot, MX_X64_RAX, cc->ip[6]);
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, ra, MX_X64_RAX);
        }
    }
    if (form == MX_JIT_FORM_IR) {
        return mx_jit_edges(cc, plan, swapped);
    } else {
        return mx_jit_edges(cc, plan, rel);
    }
}

uint32_t mx_jit_br_f(mx_jit_cc_t *cc, uint32_t rel, mx_jit_form_t form) {
    uint32_t reg = mx_jit_use_x(cc, (uint32_t)cc->ip[5]);
    uint32_t left;
    uint32_t right;
    if (form == MX_JIT_FORM_RR) {
        left = reg;
        right = mx_jit_use_x(cc, (uint32_t)cc->ip[6]);
    } else if (form == MX_JIT_FORM_RI) {
        mx_jit_load_lit_x(cc, MX_JIT_XMM_SCRATCH, cc->ip[6]);
        left = reg;
        right = MX_JIT_XMM_SCRATCH;
    } else {
        mx_jit_load_lit_x(cc, MX_JIT_XMM_SCRATCH, cc->ip[6]);
        left = MX_JIT_XMM_SCRATCH;
        right = reg;
    }
    if (rel == 2) {
        return mx_jit_br_eq_f(cc, left, right);
    }
    mx_jit_plan_t plan = mx_jit_plan(cc);
    mx_jit_flush_plan(cc, plan);
    mx_x64_ucomisd_rr(&cc->hot, right, left);
    if (rel == 0) {
        return mx_jit_edges(cc, plan, MX_X64_CC_A);
    } else {
        return mx_jit_edges(cc, plan, MX_X64_CC_AE);
    }
}

uint32_t mx_jit_br_bool(mx_jit_cc_t *cc) {
    mx_jit_plan_t plan = mx_jit_plan(cc);
    uint32_t reg = (uint32_t)cc->ip[5];
    if (mx_jit_is_cached(cc, reg)) {
        mx_x64_reg_t r = mx_jit_use_g(cc, reg);
        mx_jit_flush_plan(cc, plan);
        mx_x64_test_rr(&cc->hot, r, r);
    } else {
        mx_jit_flush_plan(cc, plan);
        mx_x64_cmp_mi(&cc->hot, MX_X64_RBX, mx_jit_disp(reg), 0);
    }
    return mx_jit_edges(cc, plan, MX_X64_CC_NE);
}

uint32_t mx_jit_br_rt(mx_jit_cc_t *cc, mx_hop_t hop) {
    mx_jit_plan_t plan = mx_jit_plan(cc);
    mx_jit_flush(cc);
    mx_jit_call_rt(cc, &cc->hot, MX_JIT_SLOT_CMP, hop);
    mx_jit_drop_caller_saved(cc);
    mx_x64_test8_al(&cc->hot);
    return mx_jit_edges(cc, plan, MX_X64_CC_NE);
}

void mx_jit_len_table(mx_jit_cc_t *cc) {
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)cc->ip[2]);
    uint8_t *exit = mx_jit_exit_stub(cc);
    mx_x64_test8_mi(&cc->hot, t, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, exit);
    mx_x64_cmp_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, meta), 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    mx_x64_reg_t d = mx_jit_def_g(cc, (uint32_t)cc->ip[1]);
    mx_x64_load32(&cc->hot, d, t, (int32_t)offsetof(mx_obj_table_t, len));
}

bool mx_jit_kind_is_ref(mx_kind_t kind) {
    return kind == MX_KIND_STRING || kind == MX_KIND_CLOSURE || kind == MX_KIND_ERROR || kind >= MX_KIND_FIRST_DATA;
}

void mx_jit_barrier(mx_jit_cc_t *cc, mx_x64_reg_t t, mx_kind_t kind, uint8_t *stub) {
    if (mx_jit_kind_is_ref(kind)) {
        const mx_jit_bit_t *bit = &cc->vm->jit.layout.head_barrier;
        mx_x64_test8_mi(&cc->hot, t, (int32_t)bit->off, bit->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
}

uint32_t mx_jit_size_for(uint64_t hint) {
    uint32_t s = 0;
    while (((uint64_t)MX_TABLE_HASH_MIN_CAP << s) <= hint) {
        s += 1;
    }
    return s;
}

void mx_jit_check_slot(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint64_t key, uint8_t *miss) {
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    int32_t at = (int32_t)(hint * sizeof(mx_table_entry_t));
    uint32_t mask = (uint32_t)layout->table_plain.mask | layout->table_size.mask;
    uint32_t want = (uint32_t)layout->table_plain.mask | mx_jit_size_for(hint);
    mx_x64_movzx8(&cc->hot, MX_X64_RAX, t, (int32_t)layout->table_plain.off);
    mx_x64_and32_ri(&cc->hot, MX_X64_RAX, (int32_t)mask);
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RAX, (int32_t)want);
    mx_x64_jcc(&cc->hot, MX_X64_CC_B, miss);
    mx_x64_load(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, miss);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key_kind), MX_KIND_STRING);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key), MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
}

uint32_t mx_jit_size_max(uint64_t hint, uint32_t hash) {
    uint32_t s = mx_jit_size_for(hint);
    while (s < 31 && ((uint64_t)hash & (((uint64_t)MX_TABLE_HASH_MIN_CAP << (s + 1)) - 1)) == hint) {
        s += 1;
    }
    return s;
}

void mx_jit_insert_key(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint64_t key, mx_kind_t kind, const uintptr_t *arg, uint8_t *stub) {
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    uint32_t hash = ((const mx_str_t *)(uintptr_t)key)->hash;
    int32_t at = (int32_t)(hint * sizeof(mx_table_entry_t));
    if (hash == 0 || ((uint64_t)hash & (((uint64_t)MX_TABLE_HASH_MIN_CAP << mx_jit_size_for(hint)) - 1)) != hint) {
        mx_x64_jmp(&cc->hot, stub);
        return;
    }
    mx_x64_movzx8(&cc->hot, MX_X64_RDX, t, (int32_t)layout->table_size.off);
    mx_x64_and32_ri(&cc->hot, MX_X64_RDX, layout->table_size.mask);
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RDX, (int32_t)mx_jit_size_max(hint, hash));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, stub);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, (uint64_t)(uintptr_t)cc->vm->jit.room);
    mx_x64_load_x(&cc->hot, MX_X64_RAX, MX_X64_RAX, MX_X64_RDX, 0);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, used));
    mx_x64_jcc(&cc->hot, MX_X64_CC_BE, stub);
    mx_x64_load32(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, used));
    mx_x64_lea32(&cc->hot, MX_X64_RAX, MX_X64_RAX, 1);
    mx_x64_store32(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, used), MX_X64_RAX);
    mx_x64_store16_i(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key_kind), MX_KIND_STRING);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key);
    mx_x64_store(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key), MX_X64_RAX);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, hash);
    mx_x64_store32(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, hash), MX_X64_RAX);
    mx_x64_store16_i(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val_kind), kind);
    mx_jit_store_arg(cc, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val), arg, MX_X64_RAX);
}

void mx_jit_check_entries(mx_jit_cc_t *cc, mx_x64_reg_t t, uint64_t hint, uint8_t *miss) {
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    uint32_t mask = (uint32_t)layout->table_plain.mask | layout->table_size.mask;
    uint32_t want = (uint32_t)layout->table_plain.mask | mx_jit_size_for(hint);
    mx_x64_movzx8(&cc->hot, MX_X64_RAX, t, (int32_t)layout->table_plain.off);
    mx_x64_and32_ri(&cc->hot, MX_X64_RAX, (int32_t)mask);
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RAX, (int32_t)want);
    mx_x64_jcc(&cc->hot, MX_X64_CC_B, miss);
    mx_x64_load(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, miss);
}

void mx_jit_set_sk(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[1]);
    uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_TABLE_SET_SK);
    mx_kind_t kind = (mx_kind_t)(ip[4] >> 1);
    uint64_t hint = ip[6];
    if (kind == MX_KIND_NIL || hint >= ((uint64_t)1 << 24)) {
        mx_x64_jmp(&cc->hot, stub);
    } else {
        int32_t at = (int32_t)(hint * sizeof(mx_table_entry_t));
        int32_t key_kind_at = at + (int32_t)offsetof(mx_table_entry_t, key_kind);
        mx_jit_check_entries(cc, t, hint, stub);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, key_kind_at, MX_KIND_NIL);
        uint8_t *empty = mx_jit_label(&cc->hot, MX_X64_CC_E);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, key_kind_at, MX_KIND_STRING);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[2]);
        mx_x64_cmp_mr(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key), MX_X64_RAX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val_kind), MX_KIND_NIL);
        mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
        mx_jit_barrier(cc, t, kind, stub);
        mx_x64_store16_i(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val_kind), kind);
        mx_jit_store_arg(cc, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val), ip + 3, MX_X64_RAX);
        uint8_t *done = mx_jit_label_jmp(&cc->hot);
        mx_x64_t hot = cc->hot;
        cc->hot = cc->cold;
        mx_x64_patch(empty, cc->hot.p);
        mx_x64_cmp_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, meta), 0);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        const mx_jit_bit_t *barrier = &cc->vm->jit.layout.head_barrier;
        mx_x64_test8_mi(&cc->hot, t, (int32_t)barrier->off, barrier->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_jit_insert_key(cc, t, hint, ip[2], kind, ip + 3, stub);
        uint8_t *back = mx_jit_label_jmp(&cc->hot);
        cc->cold = cc->hot;
        cc->hot = hot;
        mx_x64_patch(done, cc->hot.p);
        mx_x64_patch(back, cc->hot.p);
    }
    mx_jit_slow_join(cc);
}

void mx_jit_init_sk(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[1]);
    uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_TABLE_INIT_SK);
    mx_kind_t kind = (mx_kind_t)(ip[4] >> 1);
    uint64_t hint = ip[5];
    if (kind == MX_KIND_NIL || hint >= ((uint64_t)1 << 24)) {
        mx_x64_jmp(&cc->hot, stub);
    } else {
        int32_t at = (int32_t)(hint * sizeof(mx_table_entry_t));
        const mx_jit_bit_t *barrier = &cc->vm->jit.layout.head_barrier;
        mx_jit_check_entries(cc, t, hint, stub);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, key_kind), MX_KIND_NIL);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_x64_test8_mi(&cc->hot, t, (int32_t)barrier->off, barrier->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_jit_insert_key(cc, t, hint, ip[2], kind, ip + 3, stub);
    }
    mx_jit_slow_join(cc);
}

void mx_jit_init_ik(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_jit_bit_t *hash = &cc->vm->jit.layout.table_int_hash;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[1]);
    uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_TABLE_INIT_IK);
    mx_kind_t kind = (mx_kind_t)(ip[5] >> 1);
    uint64_t at = ip[2] - 1;
    if (kind == MX_KIND_NIL || at >= ((uint64_t)1 << 24)) {
        mx_x64_jmp(&cc->hot, stub);
    } else {
        int32_t disp = (int32_t)(at * sizeof(mx_obj_t));
        mx_x64_cmp32_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_len), (int32_t)at);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_x64_cmp32_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_cap), (int32_t)at);
        mx_x64_jcc(&cc->hot, MX_X64_CC_BE, stub);
        mx_x64_test8_mi(&cc->hot, t, (int32_t)hash->off, hash->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        mx_x64_load(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
        mx_x64_store16_i(&cc->hot, MX_X64_RAX, disp + (int32_t)offsetof(mx_obj_t, kind), kind);
        mx_jit_store_arg(cc, MX_X64_RAX, disp + (int32_t)offsetof(mx_obj_t, val), ip + 4, MX_X64_RCX);
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, at + 1);
        mx_x64_store32(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_len), MX_X64_RAX);
        mx_x64_cmp32_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, len), (int32_t)at);
        uint8_t *other = mx_jit_label(&cc->hot, MX_X64_CC_NE);
        mx_x64_store32(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, len), MX_X64_RAX);
        mx_x64_patch(other, cc->hot.p);
    }
    mx_jit_slow_join(cc);
}

void mx_jit_set_int(mx_jit_cc_t *cc, mx_hop_t hop) {
    uintptr_t *ip = cc->ip;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[1]);
    mx_x64_reg_t k = 0;
    if (hop == MX_HOP_TABLE_SET_IR) {
        k = mx_jit_use_g(cc, (uint32_t)ip[2]);
    }
    uint8_t *stub = mx_jit_slow_stub(cc, hop);
    mx_kind_t kind = (mx_kind_t)(ip[4] >> 1);
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    if (kind == MX_KIND_NIL || (hop == MX_HOP_TABLE_SET_IK && ip[2] - 1 >= ((uint64_t)1 << 24))) {
        mx_x64_jmp(&cc->hot, stub);
        mx_jit_slow_join(cc);
        return;
    }
    mx_x64_cmp_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, meta), 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    int32_t disp;
    uint8_t *join = NULL;
    if (hop == MX_HOP_TABLE_SET_IR) {
        mx_x64_lea(&cc->hot, MX_X64_RAX, k, -1);
        mx_x64_load32(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, array_len));
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_AE, cc->cold.p);
        join = mx_jit_append(cc, t, kind, ip + 3, stub);
        mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 4);
        mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
        disp = 0;
    } else {
        mx_x64_cmp32_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_len), (int32_t)(ip[2] - 1));
        mx_x64_jcc(&cc->hot, MX_X64_CC_BE, stub);
        mx_x64_load(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
        disp = (int32_t)((ip[2] - 1) * sizeof(mx_obj_t));
    }
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, disp + kind_at, MX_KIND_NIL);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_jit_barrier(cc, t, kind, stub);
    mx_x64_store16_i(&cc->hot, MX_X64_RAX, disp + kind_at, kind);
    mx_jit_store_arg(cc, MX_X64_RAX, disp + val_at, ip + 3, MX_X64_RCX);
    mx_jit_slow_join(cc);
    if (join != NULL) {
        mx_x64_patch(join, cc->hot.p);
    }
}

void mx_jit_set_var(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_jit_bit_t *plain = &cc->vm->jit.layout.table_plain;
    bool is_table_reg = (ip[2] & 1) == 0 && (mx_kind_t)(ip[2] >> 1) == cc->vm->kinds.table;
    bool is_str_reg = (ip[4] & 1) == 0 && (mx_kind_t)(ip[4] >> 1) == MX_KIND_STRING;
    mx_kind_t kind = (mx_kind_t)(ip[6] >> 1);
    if (!is_table_reg || !is_str_reg || kind == MX_KIND_NIL) {
        uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_TABLE_SET);
        mx_x64_jmp(&cc->hot, stub);
        mx_jit_slow_join(cc);
        return;
    }
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[1]);
    mx_x64_reg_t k = mx_jit_use_g(cc, (uint32_t)ip[3]);
    uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_TABLE_SET);
    mx_x64_test8_mi(&cc->hot, t, (int32_t)plain->off, plain->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_x64_mov_rr(&cc->hot, MX_X64_RAX, t);
    mx_x64_mov_rr(&cc->hot, MX_X64_RDX, k);
    mx_x64_call_rip(&cc->hot, &cc->vm->jit.slots[MX_JIT_SLOT_PROBE]);
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RAX, 1);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind), MX_KIND_NIL);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_jit_barrier(cc, t, kind, stub);
    mx_x64_store16_i(&cc->hot, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind), kind);
    mx_jit_store_arg(cc, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val), ip + 5, MX_X64_RAX);
    mx_jit_slow_join(cc);
}

uint8_t *mx_jit_append(mx_jit_cc_t *cc, mx_x64_reg_t t, mx_kind_t kind, const uintptr_t *arg, uint8_t *stub) {
    mx_x64_t hot = cc->hot;
    cc->hot = cc->cold;
    const mx_jit_bit_t *hash = &cc->vm->jit.layout.table_int_hash;
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array_cap));
    mx_x64_jcc(&cc->hot, MX_X64_CC_AE, stub);
    mx_x64_test8_mi(&cc->hot, t, (int32_t)hash->off, hash->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_jit_barrier(cc, t, kind, stub);
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RAX);
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 4);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
    mx_x64_store16_i(&cc->hot, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, kind), kind);
    mx_jit_store_arg(cc, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val), arg, MX_X64_RDX);
    mx_x64_lea32(&cc->hot, MX_X64_RAX, MX_X64_RCX, 1);
    mx_x64_store32(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_len), MX_X64_RAX);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, len));
    uint8_t *other = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_x64_store32(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, len), MX_X64_RAX);
    mx_x64_patch(other, cc->hot.p);
    uint8_t *join = mx_jit_label_jmp(&cc->hot);
    cc->cold = cc->hot;
    cc->hot = hot;
    return join;
}

bool mx_jit_has_hot(const uintptr_t *hot) {
    if (hot[0] == MX_KIND_MIXED || hot[0] == MX_KIND_NIL || hot[1] == 0) {
        return false;
    }
    const uintptr_t *code = (const uintptr_t *)hot[1];
    return !((const mx_version_t *)code[-1])->is_hooked;
}

uint32_t mx_jit_finish_get(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *word, mx_kind_t kind, mx_x64_reg_t src, int32_t disp) {
    uintptr_t *code = (uintptr_t *)*word;
    if (mx_jit_can_stitch(cc, code)) {
        if (kind == MX_KIND_FLOAT) {
            uint32_t x = mx_jit_def_x(cc, out);
            mx_x64_movsd_load(&cc->hot, x, src, disp);
        } else {
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_load(&cc->hot, d, src, disp);
        }
        return mx_jit_continue(cc, code);
    }
    const uint64_t *live = mx_jit_live(cc, rec->target);
    mx_x64_load(&cc->hot, MX_X64_RAX, src, disp);
    mx_jit_flush_live(cc, &cc->hot, out, live, live);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(out), MX_X64_RAX);
    mx_jit_edge_jmp(cc, word, 0);
    return 0;
}

uint32_t mx_jit_finish_rax(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *word, mx_kind_t kind) {
    uintptr_t *code = (uintptr_t *)*word;
    if (mx_jit_can_stitch(cc, code)) {
        if (kind == MX_KIND_FLOAT) {
            uint32_t x = mx_jit_def_x(cc, out);
            mx_x64_movq_xr(&cc->hot, x, MX_X64_RAX);
        } else {
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
        }
        return mx_jit_continue(cc, code);
    }
    const uint64_t *live = mx_jit_live(cc, rec->target);
    mx_jit_flush_live(cc, &cc->hot, out, live, live);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(out), MX_X64_RAX);
    mx_jit_edge_jmp(cc, word, 0);
    return 0;
}

mx_jit_way_t mx_jit_value_way(const mx_resume_t *rec) {
    uintptr_t key = rec->cache.key;
    uintptr_t code = rec->cache.code;
    for (uint32_t i = 0; i < MX_RESUME_WAYS; i++) {
        uintptr_t flags = key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED);
        if (code != 0 && (key >> 48) == 2 && flags == 0 && (mx_kind_t)key != MX_KIND_NIL) {
            return (mx_jit_way_t) { .code = (uintptr_t *)code, .kind = (mx_kind_t)key };
        } else if (i + 1 < MX_RESUME_WAYS && rec->cache.more != NULL) {
            key = rec->cache.more[2 * i];
            code = rec->cache.more[2 * i + 1];
        } else {
            code = 0;
        }
    }
    return (mx_jit_way_t) { .code = NULL, .kind = MX_KIND_NIL };
}

mx_jit_way_t mx_jit_any_way(const mx_resume_t *rec) {
    uintptr_t key = rec->cache.key;
    uintptr_t code = rec->cache.code;
    for (uint32_t i = 0; i < MX_RESUME_WAYS; i++) {
        uintptr_t flags = key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED);
        if (code != 0 && mx_jit_key_len(key) == 2 && flags == 0) {
            return (mx_jit_way_t) { .code = (uintptr_t *)code, .kind = (mx_kind_t)key };
        } else if (i + 1 < MX_RESUME_WAYS && rec->cache.more != NULL) {
            key = rec->cache.more[2 * i];
            code = rec->cache.more[2 * i + 1];
        } else {
            code = 0;
        }
    }
    return (mx_jit_way_t) { .code = NULL, .kind = MX_KIND_NIL };
}

void mx_jit_cold_nil(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, uintptr_t *code) {
    mx_x64_mov_ri(&cc->cold, MX_X64_RAX, 0);
    if (mx_jit_can_stitch(cc, code)) {
        uint8_t *site = mx_jit_label_jmp(&cc->cold);
        mx_jit_push_pend(cc, code, site);
        mx_jit_pend_t *p = &cc->pend[cc->npend - 1];
        p->has_val = true;
        p->val_out = out;
        p->val_kind = MX_KIND_NIL;
    } else {
        const uint64_t *live = mx_jit_live(cc, rec->target);
        mx_jit_flush_live(cc, &cc->cold, out, live, live);
        mx_x64_store_i32(&cc->cold, MX_X64_RBX, mx_jit_disp(out), 0);
        uint8_t *site = mx_jit_label_jmp(&cc->cold);
        uintptr_t *word = mx_jit_cell(cc, code);
        mx_jit_edge_stub(cc, site, word, 0);
    }
}

mx_jit_miss_t mx_jit_miss_seq(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_x64_reg_t t, mx_x64_reg_t k, mx_kind_t key_kind, uint64_t lit, int32_t at, uint8_t *stub) {
    mx_t *vm = cc->vm;
    mx_x64_t *a = &cc->cold;
    uint8_t *start = a->p;
    mx_x64_mov_rr(a, MX_X64_RAX, t);
    if (k == MX_JIT_NONE) {
        mx_x64_mov_ri(a, MX_X64_RDX, lit);
    } else {
        mx_x64_mov_rr(a, MX_X64_RDX, k);
    }
    if (key_kind == MX_KIND_STRING) {
        mx_x64_call_rip(a, &vm->jit.slots[MX_JIT_SLOT_LOOKUP]);
    } else {
        mx_x64_mov_ri(a, MX_X64_RCX, key_kind);
        mx_x64_call_rip(a, &vm->jit.slots[MX_JIT_SLOT_LOOKUP_REF]);
    }
    mx_x64_cmp32_ri(a, MX_X64_RAX, 1);
    uint8_t *found = mx_jit_label(a, MX_X64_CC_E);
    uintptr_t *nil_code = mx_run_cache_find(&rec->cache, (uintptr_t)2 << 48);
    if (nil_code == NULL || rec->has_cell_out) {
        mx_x64_jmp(a, stub);
    } else {
        mx_x64_cmp32_ri(a, MX_X64_RAX, 0);
        mx_x64_jcc(a, MX_X64_CC_NE, stub);
        mx_jit_cold_nil(cc, rec, out, nil_code);
    }
    mx_x64_patch(found, a->p);
    if (at != 0) {
        mx_x64_lea(a, MX_X64_RCX, MX_X64_RCX, -at);
    }
    uint8_t *site = mx_jit_label_jmp(a);
    return (mx_jit_miss_t) { .start = start, .found = site };
}

uint32_t mx_jit_get_sk(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_resume_t *rec = (const mx_resume_t *)ip[4];
    uint32_t out = (uint32_t)ip[1];
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[2]);
    uint8_t *stub = mx_jit_get_stub(cc, MX_HOP_TABLE_GET_SK);
    uint64_t hint = ip[5];
    bool has_nil = mx_run_cache_find(&rec->cache, (uintptr_t)2 << 48) != NULL;
    mx_jit_way_t way = mx_jit_value_way(rec);
    bool has_hot = mx_jit_has_hot(ip + 6);
    bool has_value = !rec->has_cell_out && (has_hot || way.code != NULL);
    if (!has_value && !has_nil) {
        mx_x64_jmp(&cc->hot, stub);
        return 0;
    } else if (!has_value) {
        mx_jit_miss_t miss = mx_jit_miss_seq(cc, rec, out, t, MX_JIT_NONE, MX_KIND_STRING, ip[3], 0, stub);
        mx_x64_patch(miss.found, stub);
        mx_x64_jmp(&cc->hot, miss.start);
        return 0;
    }
    bool has_hint = hint < ((uint64_t)1 << 24);
    int32_t at = 0;
    if (has_hint) {
        at = (int32_t)(hint * sizeof(mx_table_entry_t));
    }
    mx_jit_miss_t miss = mx_jit_miss_seq(cc, rec, out, t, MX_JIT_NONE, MX_KIND_STRING, ip[3], at, stub);
    if (has_hint) {
        mx_jit_check_slot(cc, t, hint, ip[3], miss.start);
    } else {
        mx_x64_jmp(&cc->hot, miss.start);
    }
    mx_x64_patch(miss.found, cc->hot.p);
    uintptr_t *word;
    mx_kind_t kind;
    if (has_hot) {
        word = &ip[7];
        kind = (mx_kind_t)ip[6];
    } else {
        word = mx_jit_cell(cc, way.code);
        kind = way.kind;
    }
    mx_jit_kind_ways(cc, rec, out, kind, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val_kind), at + (int32_t)offsetof(mx_table_entry_t, val), stub, 0);
    return mx_jit_finish_get(cc, rec, out, word, kind, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val));
}

uint32_t mx_jit_get_int(mx_jit_cc_t *cc, mx_hop_t hop) {
    uintptr_t *ip = cc->ip;
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[2]);
    mx_x64_reg_t k = 0;
    if (hop == MX_HOP_TABLE_GET_IR) {
        k = mx_jit_use_g(cc, (uint32_t)ip[3]);
    }
    uint8_t *stub = mx_jit_get_stub(cc, hop);
    if (!mx_jit_has_hot(ip + 5) || (hop == MX_HOP_TABLE_GET_IK && ip[3] - 1 >= ((uint64_t)1 << 24))) {
        mx_x64_jmp(&cc->hot, stub);
        return 0;
    }
    mx_x64_test8_mi(&cc->hot, t, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    int32_t disp;
    if (hop == MX_HOP_TABLE_GET_IR) {
        mx_x64_lea(&cc->hot, MX_X64_RAX, k, -1);
        mx_x64_load32(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, array_len));
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_AE, stub);
        mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 4);
        mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
        disp = 0;
    } else {
        mx_x64_cmp32_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, array_len), (int32_t)(ip[3] - 1));
        mx_x64_jcc(&cc->hot, MX_X64_CC_BE, stub);
        mx_x64_load(&cc->hot, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
        disp = (int32_t)((ip[3] - 1) * sizeof(mx_obj_t));
    }
    mx_jit_kind_ways(cc, (const mx_resume_t *)ip[4], (uint32_t)ip[1], (mx_kind_t)ip[5], MX_X64_RAX, disp + (int32_t)offsetof(mx_obj_t, kind), disp + (int32_t)offsetof(mx_obj_t, val), stub, 0);
    return mx_jit_finish_get(cc, (const mx_resume_t *)ip[4], (uint32_t)ip[1], &ip[6], (mx_kind_t)ip[5], MX_X64_RAX, disp + (int32_t)offsetof(mx_obj_t, val));
}

uint32_t mx_jit_method_get(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    uint8_t *stub = mx_jit_get_stub(cc, MX_HOP_METHOD_GET_SK);
    uint64_t hint = ip[5];
    if (!mx_jit_has_hot(ip + 7) || hint >= ((uint64_t)1 << 24)) {
        mx_x64_jmp(&cc->hot, stub);
        return 0;
    }
    int32_t at = (int32_t)(hint * sizeof(mx_table_entry_t));
    int32_t methods = (int32_t)((mx_kind_t)ip[6] * sizeof(mx_kind_info_t) + offsetof(mx_kind_info_t, get_methods));
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)(offsetof(mx_t, kinds) + offsetof(mx_kinds_t, ptr)));
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, methods + (int32_t)offsetof(mx_obj_t, kind), cc->vm->kinds.table);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_load(&cc->hot, MX_X64_RDX, MX_X64_RAX, methods + (int32_t)offsetof(mx_obj_t, val));
    mx_jit_check_slot(cc, MX_X64_RDX, hint, ip[3], stub);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val_kind), (uint16_t)ip[7]);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    return mx_jit_finish_get(cc, (const mx_resume_t *)ip[4], (uint32_t)ip[1], &ip[8], (mx_kind_t)ip[7], MX_X64_RCX, at + (int32_t)offsetof(mx_table_entry_t, val));
}

void mx_jit_upval_load(mx_jit_cc_t *cc, mx_x64_reg_t cl, uint32_t index, mx_kind_t kind, uint8_t *miss) {
    int32_t value_kind = (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, kind));
    int32_t value_val = (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, val));
    mx_x64_load(&cc->hot, MX_X64_RAX, cl, (int32_t)(offsetof(mx_obj_closure_t, upvals) + index * sizeof(mx_obj_upval_t *)));
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, thread));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    uint8_t *open = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, value_kind, kind);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RAX, value_val);
    uint8_t *done = mx_jit_label_jmp(&cc->hot);
    mx_x64_patch(open, cc->hot.p);
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RCX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_load32(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, kind_at));
    mx_x64_movzx16_x(&cc->hot, MX_X64_RDX, MX_X64_RCX, MX_X64_RDX, 0);
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RDX, kind);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
    mx_x64_load32(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, at));
    mx_x64_load_x(&cc->hot, MX_X64_RAX, MX_X64_RCX, MX_X64_RDX, 0);
    mx_x64_patch(done, cc->hot.p);
}

uint32_t mx_jit_upval_get(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    if (cc->rb->block->func->has_cells) {
        mx_jit_flush(cc);
    }
    mx_x64_reg_t cl = mx_jit_use_g(cc, 0);
    uint8_t *stub = mx_jit_get_stub(cc, MX_HOP_UPVAL_GET);
    const mx_resume_t *rec = (const mx_resume_t *)ip[3];
    uint32_t out = (uint32_t)ip[1];
    uintptr_t *word;
    mx_kind_t kind;
    if (mx_jit_has_hot(ip + 5)) {
        word = &ip[6];
        kind = (mx_kind_t)ip[5];
    } else {
        mx_jit_way_t way = mx_jit_any_way(rec);
        if (way.code == NULL || rec->has_cell_out) {
            mx_x64_jmp(&cc->hot, stub);
            return 0;
        }
        word = mx_jit_cell(cc, way.code);
        kind = way.kind;
    }
    mx_jit_upval_fetch(cc, cl, (uint32_t)ip[2]);
    mx_jit_reg_ways(cc, rec, out, kind, stub);
    return mx_jit_finish_rax(cc, rec, out, word, kind);
}

void mx_jit_upval_fetch(mx_jit_cc_t *cc, mx_x64_reg_t cl, uint32_t index) {
    mx_x64_load(&cc->hot, MX_X64_RAX, cl, (int32_t)(offsetof(mx_obj_closure_t, upvals) + index * sizeof(mx_obj_upval_t *)));
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, thread));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    uint8_t *open = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_x64_movzx16(&cc->hot, MX_X64_RCX, MX_X64_RAX, (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, kind)));
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RAX, (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, val)));
    uint8_t *done = mx_jit_label_jmp(&cc->hot);
    mx_x64_patch(open, cc->hot.p);
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RCX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_load32(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, kind_at));
    mx_x64_movzx16_x(&cc->hot, MX_X64_RDX, MX_X64_RCX, MX_X64_RDX, 0);
    mx_x64_load32(&cc->hot, MX_X64_RAX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, at));
    mx_x64_load_x(&cc->hot, MX_X64_RAX, MX_X64_RCX, MX_X64_RAX, 0);
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RDX);
    mx_x64_patch(done, cc->hot.p);
}

void mx_jit_reg_ways(mx_jit_cc_t *cc, const mx_resume_t *rec, uint32_t out, mx_kind_t hot, uint8_t *stub) {
    uintptr_t keys[MX_RESUME_WAYS];
    uintptr_t *codes[MX_RESUME_WAYS];
    uint32_t n = 0;
    keys[n] = rec->cache.key;
    codes[n] = (uintptr_t *)rec->cache.code;
    n += 1;
    for (uint32_t i = 0; i + 1 < MX_RESUME_WAYS && rec->cache.more != NULL; i++) {
        keys[n] = rec->cache.more[2 * i];
        codes[n] = (uintptr_t *)rec->cache.more[2 * i + 1];
        n += 1;
    }
    uint8_t *miss = cc->cold.p;
    bool has_way = false;
    for (uint32_t i = 0; i < n; i++) {
        mx_kind_t kind = (mx_kind_t)keys[i];
        bool is_value = codes[i] != NULL && mx_jit_key_len(keys[i]) == 2 && (keys[i] & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED)) == 0;
        if (is_value && kind != hot && mx_jit_can_stitch(cc, codes[i])) {
            mx_x64_cmp32_ri(&cc->cold, MX_X64_RCX, kind);
            uint8_t *other = mx_jit_label(&cc->cold, MX_X64_CC_NE);
            uint8_t *site = mx_jit_label_jmp(&cc->cold);
            mx_jit_push_pend(cc, codes[i], site);
            mx_jit_pend_t *p = &cc->pend[cc->npend - 1];
            p->has_val = true;
            p->val_out = out;
            p->val_kind = kind;
            mx_x64_patch(other, cc->cold.p);
            has_way = true;
        }
    }
    mx_x64_cmp32_ri(&cc->hot, MX_X64_RCX, hot);
    if (has_way) {
        mx_x64_jmp(&cc->cold, stub);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss);
    } else {
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
}

void mx_jit_upval_guard(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    if (cc->rb->block->func->has_cells) {
        mx_jit_flush(cc);
    }
    mx_x64_reg_t cl = mx_jit_use_g(cc, 0);
    mx_jit_upval_load(cc, cl, (uint32_t)ip[2], (mx_kind_t)ip[3], mx_jit_exit_stub(cc));
    mx_x64_reg_t d = mx_jit_def_g(cc, (uint32_t)ip[1]);
    mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
}

bool mx_jit_can_inline(const mx_jit_cc_t *cc, const mx_resume_t *rec, const uintptr_t *code) {
    const mx_version_t *crb = (const mx_version_t *)code[-1];
    const mx_ir_func_t *func = crb->block->func;
    if (cc->ninline >= MX_JIT_INLINE_DEPTH || cc->inlined >= MX_JIT_INLINE_MAX || crb->is_hooked || func->nblocks > MX_JIT_INLINE_BLOCKS || cc->rb->block->func->has_cells) {
        return false;
    } else if (rec->kind != MX_RESUME_VALUES || rec->want > 3 || rec->has_cell_out || crb->start_at != 0) {
        return false;
    } else if (func == cc->rb->block->func || func == ((const mx_version_t *)cc->first[-1])->block->func) {
        return false;
    }
    for (uint32_t i = 0; i < cc->ninline; i++) {
        if (cc->stack[i].rb->block->func == func) {
            return false;
        }
    }
    return code[1] != cc->vm->jit.back || code[2] >= cc->vm->jit.threshold;
}

uint32_t mx_jit_call_inline(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *stub) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    mx_kind_t skind = (mx_kind_t)((key >> 48) - 1);
    mx_version_t *crb = (mx_version_t *)code[-1];
    uint32_t np = crb->nparams;
    uint32_t k = cc->frame + 2;
    uint32_t above = k + crb->frame + 2 + 2 * (n + (uint32_t)is_spread) + MX_RUN_SLACK;
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && cc->g[i].reg >= cc->off + cc->frame) {
            cc->g[i].is_used = false;
            cc->g[i].is_dirty = false;
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used && cc->x[i].reg >= cc->off + cc->frame) {
            cc->x[i].is_used = false;
            cc->x[i].is_dirty = false;
        }
    }
    mx_x64_mov_ri(&cc->hot, MX_X64_RCX, key & MX_JIT_KEY_BLOCK);
    mx_x64_cmp_mr(&cc->hot, fn, (int32_t)offsetof(mx_obj_closure_t, block), MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    if (is_spread) {
        mx_jit_spread_guard(cc, skind, stub);
    }
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RBX);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SAR, MX_X64_RCX, 3);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RCX, (int32_t)above);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, limit));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, stub);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, cap));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, stub);
    mx_x64_alu_ri(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, 2);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(cc->frame), MX_X64_RCX);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[4]);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(cc->frame + 1), MX_X64_RAX);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(k), fn);
    mx_jit_unpin(cc);
    for (uint32_t i = 0; i < np; i++) {
        uint32_t dst = k + 1 + i;
        if (is_spread && i == n) {
            int32_t vals_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals));
            mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
            if (skind == MX_KIND_FLOAT) {
                uint32_t d = mx_jit_def_x(cc, dst);
                mx_x64_movsd_load(&cc->hot, d, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val));
            } else {
                mx_x64_reg_t d = mx_jit_def_g(cc, dst);
                mx_x64_load(&cc->hot, d, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val));
            }
        } else if (i >= n) {
            mx_jit_move_i(cc, dst, 0);
        } else if ((ip[8 + 2 * i + 1] & 1) != 0) {
            mx_jit_move_i(cc, dst, ip[8 + 2 * i]);
        } else {
            mx_jit_move_r(cc, dst, (uint32_t)ip[8 + 2 * i]);
        }
        mx_jit_unpin(cc);
    }
    cc->stack[cc->ninline] = (mx_jit_inline_t) {
        .rec = (const mx_resume_t *)ip[4],
        .rb = cc->rb,
        .code = cc->code,
        .tail = cc->tail,
        .link_tail = cc->link_tail,
        .k = k,
        .nregs = cc->nregs,
        .frame = cc->frame,
        .chain = cc->chain,
    };
    cc->ninline += 1;
    cc->inlined += 1;
    cc->off += k;
    cc->rb = crb;
    cc->code = code;
    cc->ip = code;
    cc->nregs = crb->block->func->nregs;
    cc->frame = crb->frame;
    cc->next_chain += 1;
    cc->chain = cc->next_chain;
    mx_x64_lea(&cc->hot, MX_X64_RBX, MX_X64_RBX, mx_jit_disp(k));
    mx_jit_tails(cc);
    return MX_JIT_ENTRY_WORDS;
}

uint32_t mx_jit_return_inline(mx_jit_cc_t *cc, const uintptr_t *arg) {
    mx_jit_inline_t *frame = &cc->stack[cc->ninline - 1];
    const mx_resume_t *rec = frame->rec;
    uint32_t callee_abs = cc->off;
    uint32_t want = rec->want;
    mx_kind_t kind = MX_KIND_NIL;
    uint8_t g = MX_JIT_NONE;
    uint8_t x = MX_JIT_NONE;
    bool is_lit = false;
    uint64_t lit = 0;
    if (arg != NULL) {
        kind = (mx_kind_t)(arg[1] >> 1);
        if ((arg[1] & 1) != 0) {
            is_lit = true;
            lit = arg[0];
        } else {
            g = mx_jit_find_g(cc, (uint32_t)arg[0]);
            x = mx_jit_find_x(cc, (uint32_t)arg[0]);
            if (g == MX_JIT_NONE && x == MX_JIT_NONE) {
                mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RBX, mx_jit_disp((uint32_t)arg[0]));
            }
        }
    } else {
        is_lit = true;
    }
    mx_x64_lea(&cc->hot, MX_X64_RBX, MX_X64_RBX, -mx_jit_disp(frame->k));
    cc->off -= frame->k;
    cc->rb = frame->rb;
    cc->code = frame->code;
    cc->tail = frame->tail;
    cc->link_tail = frame->link_tail;
    cc->nregs = frame->nregs;
    cc->frame = frame->frame;
    cc->chain = frame->chain;
    cc->ninline -= 1;
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && cc->g[i].reg >= callee_abs - 2) {
            cc->g[i].is_used = false;
            cc->g[i].is_dirty = false;
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used && cc->x[i].reg >= callee_abs - 2) {
            cc->x[i].is_used = false;
            cc->x[i].is_dirty = false;
        }
    }
    mx_kind_t kinds[3] = { kind, MX_KIND_NIL, MX_KIND_NIL };
    if (want >= 1) {
        uint32_t out = rec->out;
        if (is_lit) {
            mx_jit_move_i(cc, out, lit);
        } else if (x != MX_JIT_NONE) {
            cc->x[x].is_pinned = true;
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_movaps(&cc->hot, d, x);
        } else if (g != MX_JIT_NONE) {
            cc->g[g].is_pinned = true;
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_mov_rr(&cc->hot, d, g);
        } else if (kind == MX_KIND_FLOAT) {
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_movq_xr(&cc->hot, d, MX_X64_RAX);
        } else {
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
        }
        mx_jit_unpin(cc);
        for (uint32_t i = 1; i < want; i++) {
            mx_jit_move_i(cc, out + i, 0);
            mx_jit_unpin(cc);
        }
    }
    uintptr_t shape = 0;
    for (uint32_t i = 0; i < want; i++) {
        shape |= (uintptr_t)kinds[i] << (16 * i);
    }
    uintptr_t key = ((uintptr_t)(want + 1) << 48) | MX_RUN_KEY_RESUMED | shape;
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_values(cc->vm, rec, kinds, want, true)->code;
        mx_run_cache_put(cc->vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    if (mx_jit_can_stitch(cc, code)) {
        return mx_jit_continue(cc, code);
    }
    const uint64_t *live = mx_jit_live(cc, rec->target);
    mx_jit_flush_live(cc, &cc->hot, UINT32_MAX, live, live);
    mx_jit_edge_jmp(cc, mx_jit_cell(cc, code), 0);
    return 0;
}

void mx_jit_spread_guard(mx_jit_cc_t *cc, mx_kind_t kind, uint8_t *stub) {
    int32_t vals_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals));
    int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
    mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, 1);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, kind), kind);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
}

uint32_t mx_jit_call_lua(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[2];
    uintptr_t key = ip[5];
    uintptr_t *code = (uintptr_t *)ip[6];
    bool is_spread = ip[3] != 0;
    if ((is_spread && (key >> 48) == 0) || n > 32 || key == 0 || (key & 1) != 0 || code == NULL || ((const mx_version_t *)code[-1])->is_vararg || ((const mx_version_t *)code[-1])->nparams > 32) {
        mx_jit_call_frame(cc, MX_JIT_SLOT_CALL_LUA);
        return 0;
    }
    mx_x64_reg_t fn = mx_jit_use_g(cc, (uint32_t)ip[1]);
    const mx_resume_cache_t *cache = (const mx_resume_cache_t *)(ip + 5);
    if (cache->more == NULL && mx_jit_can_inline(cc, (const mx_resume_t *)ip[4], code)) {
        uint8_t *stub = cc->cold.p;
        mx_jit_spill(cc, &cc->cold);
        mx_jit_frame_seq(cc, &cc->cold, MX_JIT_SLOT_CALL_LUA);
        return mx_jit_call_inline(cc, fn, key, code, stub);
    }
    mx_jit_flush(cc);
    uint8_t *helper = mx_jit_frame_stub(cc, MX_JIT_SLOT_CALL_LUA);
    uintptr_t keys[MX_RESUME_WAYS];
    uintptr_t *codes[MX_RESUME_WAYS];
    uint32_t ways = mx_jit_call_ways(cache, keys, codes, is_spread);
    for (uint32_t i = 0; i < ways; i++) {
        if (i + 1 < ways) {
            uint8_t *tramp = cc->cold.p;
            uint8_t *site = mx_jit_label_jmp(&cc->cold);
            mx_jit_lua_frame(cc, fn, keys[i], codes[i], tramp);
            mx_x64_patch(site, cc->hot.p);
        } else {
            mx_jit_lua_frame(cc, fn, keys[i], codes[i], helper);
        }
    }
    return 0;
}

uint32_t mx_jit_call_ways(const mx_resume_cache_t *cache, uintptr_t *keys, uintptr_t **codes, bool is_spread) {
    uint32_t n = 0;
    uintptr_t key = cache->key;
    uintptr_t code = cache->code;
    for (uint32_t i = 0; i < MX_RESUME_WAYS; i++) {
        const mx_version_t *crb = NULL;
        if (code != 0) {
            crb = (const mx_version_t *)((const uintptr_t *)code)[-1];
        }
        if (crb != NULL && key != 0 && (key & 1) == 0 && ((key >> 48) != 0) == is_spread && !crb->is_vararg && crb->nparams <= 32) {
            keys[n] = key;
            codes[n] = (uintptr_t *)code;
            n += 1;
        }
        if (i + 1 < MX_RESUME_WAYS && cache->more != NULL) {
            key = cache->more[2 * i];
            code = cache->more[2 * i + 1];
        } else {
            code = 0;
        }
    }
    return n;
}

void mx_jit_lua_frame(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *exit) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[2];
    bool is_spread = ip[3] != 0;
    const mx_version_t *crb = (const mx_version_t *)code[-1];
    uint32_t np = crb->nparams;
    uint32_t frame = cc->rb->frame;
    uint32_t above = frame + 2 + crb->frame + 2 + 2 * (n + (uint32_t)is_spread) + MX_RUN_SLACK;
    mx_x64_mov_ri(&cc->hot, MX_X64_RCX, key & MX_JIT_KEY_BLOCK);
    mx_x64_cmp_mr(&cc->hot, fn, (int32_t)offsetof(mx_obj_closure_t, block), MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    if (is_spread) {
        mx_jit_spread_guard(cc, (mx_kind_t)((key >> 48) - 1), exit);
    }
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RBX);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SAR, MX_X64_RCX, 3);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RCX, (int32_t)above);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, limit));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, exit);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, cap));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, exit);
    uint32_t callee = frame + 2;
    mx_x64_alu_ri(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, 2);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(frame), MX_X64_RCX);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[4]);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(frame + 1), MX_X64_RAX);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(callee), fn);
    for (uint32_t i = 0; i < np; i++) {
        if (i < n) {
            mx_jit_store_arg(cc, MX_X64_RBX, mx_jit_disp(callee + 1 + i), ip + 8 + 2 * i, MX_X64_RAX);
        } else if (is_spread && i == n) {
            mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals)));
            mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val));
            mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(callee + 1 + i), MX_X64_RAX);
        } else {
            mx_x64_store_i32(&cc->hot, MX_X64_RBX, mx_jit_disp(callee + 1 + i), 0);
        }
    }
    mx_x64_lea(&cc->hot, MX_X64_RBX, MX_X64_RBX, mx_jit_disp(callee));
    mx_jit_edge_jmp(cc, mx_jit_cell(cc, code), 1);
}

bool mx_jit_tail_overlaps(const uintptr_t *args, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        const uintptr_t *arg = args + 2 * i;
        if ((arg[1] & 1) == 0 && arg[0] >= 1 && arg[0] < i + 1) {
            return true;
        }
    }
    return false;
}

void mx_jit_tailcall_lua(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[2];
    uintptr_t key = ip[4];
    uintptr_t *code = (uintptr_t *)ip[5];
    if (ip[3] != 0 || n > 32 || key == 0 || (key & 1) != 0 || code == NULL || ((const mx_version_t *)code[-1])->is_vararg || ((const mx_version_t *)code[-1])->nparams > 32) {
        mx_jit_call_frame(cc, MX_JIT_SLOT_TAILCALL_LUA);
        return;
    }
    mx_x64_reg_t fn = mx_jit_use_g(cc, (uint32_t)ip[1]);
    mx_jit_flush(cc);
    uint8_t *helper = mx_jit_frame_stub(cc, MX_JIT_SLOT_TAILCALL_LUA);
    uintptr_t keys[MX_RESUME_WAYS];
    uintptr_t *codes[MX_RESUME_WAYS];
    uint32_t ways = mx_jit_call_ways((const mx_resume_cache_t *)(ip + 4), keys, codes, false);
    for (uint32_t i = 0; i < ways; i++) {
        if (i + 1 < ways) {
            uint8_t *tramp = cc->cold.p;
            uint8_t *site = mx_jit_label_jmp(&cc->cold);
            mx_jit_tail_frame(cc, fn, keys[i], codes[i], tramp);
            mx_x64_patch(site, cc->hot.p);
        } else {
            mx_jit_tail_frame(cc, fn, keys[i], codes[i], helper);
        }
    }
}

void mx_jit_tail_frame(mx_jit_cc_t *cc, mx_x64_reg_t fn, uintptr_t key, uintptr_t *code, uint8_t *exit) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[2];
    const mx_version_t *crb = (const mx_version_t *)code[-1];
    uint32_t np = crb->nparams;
    uint32_t above = crb->frame + 2 + 2 * n + MX_RUN_SLACK;
    mx_x64_mov_ri(&cc->hot, MX_X64_RCX, key);
    mx_x64_cmp_mr(&cc->hot, fn, (int32_t)offsetof(mx_obj_closure_t, block), MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RBX);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SAR, MX_X64_RCX, 3);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RCX, (int32_t)above);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, limit));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, exit);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, cap));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, exit);
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RBX, -16);
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 24);
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHR, MX_X64_RAX, 24);
    mx_x64_btc_i(&cc->hot, MX_X64_RAX, 63);
    mx_x64_store(&cc->hot, MX_X64_RBX, -16, MX_X64_RAX);
    const uintptr_t *args = ip + 7;
    uint32_t count = n;
    if (np < n) {
        count = np;
    }
    if (mx_jit_tail_overlaps(args, count)) {
        for (uint32_t i = 0; i < count; i++) {
            mx_jit_store_arg(cc, MX_X64_RBX, mx_jit_disp(cc->frame + 2 + i), args + 2 * i, MX_X64_RAX);
        }
        for (uint32_t i = 0; i < count; i++) {
            mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RBX, mx_jit_disp(cc->frame + 2 + i));
            mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(1 + i), MX_X64_RAX);
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            mx_jit_store_arg(cc, MX_X64_RBX, mx_jit_disp(1 + i), args + 2 * i, MX_X64_RAX);
        }
    }
    for (uint32_t i = count; i < np; i++) {
        mx_x64_store_i32(&cc->hot, MX_X64_RBX, mx_jit_disp(1 + i), 0);
    }
    mx_x64_store(&cc->hot, MX_X64_RBX, 0, fn);
    mx_jit_edge_jmp(cc, mx_jit_cell(cc, code), 1);
}

void mx_jit_frame_seq(mx_jit_cc_t *cc, mx_x64_t *a, mx_jit_slot_t slot) {
    mx_jit_slot_t tramp;
    if (slot == MX_JIT_SLOT_FFI) {
        tramp = MX_JIT_SLOT_T_FFI;
    } else if (slot == MX_JIT_SLOT_FFI_AFTER) {
        tramp = MX_JIT_SLOT_T_FFI_AFTER;
    } else if (slot == MX_JIT_SLOT_CALL_LUA) {
        tramp = MX_JIT_SLOT_T_CALL_LUA;
    } else {
        tramp = MX_JIT_SLOT_T_TAILCALL_LUA;
    }
    mx_x64_mov_ri(a, MX_X64_RDX, (uint64_t)(uintptr_t)cc->ip);
    mx_x64_mov_ri(a, MX_X64_RCX, (uint64_t)(uintptr_t)cc->rb);
    mx_x64_jmp(a, cc->vm->jit.slots[tramp]);
}

uint8_t *mx_jit_frame_stub(mx_jit_cc_t *cc, mx_jit_slot_t slot) {
    uint8_t *stub = cc->cold.p;
    mx_jit_frame_seq(cc, &cc->cold, slot);
    return stub;
}

void mx_jit_code_call(mx_jit_cc_t *cc, mx_jit_slot_t slot, mx_hop_t hop) {
    mx_jit_flush(cc);
    mx_jit_call_rt(cc, &cc->hot, slot, hop);
    mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, mx_jit_exit_stub(cc));
    mx_x64_jmp_m(&cc->hot, MX_X64_RAX, 8);
}

void mx_jit_call_frame(mx_jit_cc_t *cc, mx_jit_slot_t slot) {
    mx_jit_flush(cc);
    mx_jit_frame_seq(cc, &cc->hot, slot);
}

uint32_t mx_jit_many(mx_jit_cc_t *cc, const mx_resume_t *rec, uintptr_t key, uintptr_t *code, mx_x64_reg_t src, int32_t at, uint8_t *stub) {
    uint32_t want = rec->want;
    int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    if (want != 0) {
        mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, (int32_t)want);
        mx_x64_jcc(&cc->hot, MX_X64_CC_B, stub);
    }
    for (uint32_t i = 0; i < want; i++) {
        mx_x64_cmp16_mi(&cc->hot, src, at + (int32_t)(i * sizeof(mx_obj_t)) + kind_at, (uint16_t)(key >> (16 * i)));
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
    bool can_stitch = mx_jit_can_stitch(cc, code);
    for (uint32_t i = 0; i < want; i++) {
        int32_t from = at + (int32_t)(i * sizeof(mx_obj_t)) + val_at;
        mx_kind_t kind = (mx_kind_t)(key >> (16 * i));
        if (!can_stitch) {
            mx_x64_load(&cc->hot, MX_X64_RAX, src, from);
            mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(rec->out + i), MX_X64_RAX);
        } else if (kind == MX_KIND_FLOAT) {
            uint32_t x = mx_jit_def_x(cc, rec->out + i);
            mx_x64_movsd_load(&cc->hot, x, src, from);
        } else {
            mx_x64_reg_t d = mx_jit_def_g(cc, rec->out + i);
            mx_x64_load(&cc->hot, d, src, from);
        }
    }
    if (can_stitch) {
        return mx_jit_continue(cc, code);
    }
    mx_jit_edge_jmp(cc, mx_jit_cell(cc, code), 0);
    return 0;
}

uint8_t *mx_jit_ways_stub(mx_jit_cc_t *cc, const mx_resume_t *rec, mx_x64_reg_t src, int32_t at, uint8_t *fallback) {
    uint32_t want = rec->want;
    if (want < 2 || want > 3 || rec->has_cell_out || rec->kind != MX_RESUME_VALUES) {
        return fallback;
    }
    uint8_t *start = cc->cold.p;
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(uintptr_t)rec);
    mx_x64_lea(&cc->cold, MX_X64_RSI, src, at);
    mx_x64_call_rip(&cc->cold, &cc->vm->jit.slots[MX_JIT_SLOT_DELIVER2 + want - 2]);
    mx_x64_test_rr(&cc->cold, MX_X64_RAX, MX_X64_RAX);
    mx_x64_jcc(&cc->cold, MX_X64_CC_E, fallback);
    mx_x64_jmp_m(&cc->cold, MX_X64_RAX, 8);
    return start;
}

uint32_t mx_jit_call_ffi_result(mx_jit_cc_t *cc, const mx_resume_t *rec, int32_t at, uint8_t *after) {
    uintptr_t key = rec->cache.key;
    uintptr_t *code = (uintptr_t *)rec->cache.code;
    uint32_t want = rec->want;
    uintptr_t flags = key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED);
    uint8_t *ways = mx_jit_ways_stub(cc, rec, MX_X64_RBX, at, after);
    if (rec->kind != MX_RESUME_VALUES || want > 3 || rec->has_cell_out || code == NULL || mx_jit_key_len(key) != want + 1 || flags != MX_RUN_KEY_RESUMED) {
        mx_x64_jmp(&cc->hot, ways);
        return 0;
    } else if (want != 1) {
        return mx_jit_many(cc, rec, key, code, MX_X64_RBX, at, ways);
    }
    int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
    int32_t kind_at = at + (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = at + (int32_t)offsetof(mx_obj_t, val);
    mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, after);
    mx_jit_kind_ways(cc, rec, rec->out, (mx_kind_t)key, MX_X64_RBX, kind_at, val_at, after, MX_RUN_KEY_RESUMED);
    return mx_jit_finish_get(cc, rec, rec->out, mx_jit_cell(cc, code), (mx_kind_t)key, MX_X64_RBX, val_at);
}

uint32_t mx_jit_call_ffi(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    const mx_resume_t *rec = (const mx_resume_t *)ip[5];
    uint32_t n = (uint32_t)ip[3];
    mx_kind_t fn_kind = (mx_kind_t)(ip[2] >> 1);
    bool is_lit = (ip[2] & 1) != 0;
    bool is_spread = ip[4] != 0;
    if (n > 32 || fn_kind != MX_KIND_FFI || ip[6] == 0 || (is_lit && ip[1] != ip[6])) {
        mx_jit_call_frame(cc, MX_JIT_SLOT_FFI);
        return 0;
    }
    uint32_t total = n + (uint32_t)is_spread;
    uint32_t frame = cc->frame;
    uint32_t above = frame + 6 + 2 * total + MX_RUN_SLACK;
    int32_t objs = mx_jit_disp(frame + 4);
    int32_t pending_at = (int32_t)offsetof(mx_t, pending);
    int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
    int32_t vals_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals));
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    mx_x64_reg_t fn = 0;
    if (!is_lit) {
        fn = mx_jit_use_g(cc, (uint32_t)ip[1]);
    }
    mx_jit_flush(cc);
    uint8_t *helper = mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI);
    if (!is_lit) {
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[6]);
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, fn, MX_X64_RAX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, helper);
    }
    mx_x64_test8_mi(&cc->hot, MX_X64_R12, (int32_t)layout->vm_hooked.off, layout->vm_hooked.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, helper);
    if (is_spread) {
        mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, 1);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, helper);
    }
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RBX);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_SUB, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SAR, MX_X64_RCX, 3);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RCX, (int32_t)above);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, limit));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, helper);
    mx_x64_cmp32_rm(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, cap));
    mx_x64_jcc(&cc->hot, MX_X64_CC_A, helper);
    for (uint32_t i = 0; i < n; i++) {
        const uintptr_t *arg = ip + 8 + 2 * i;
        int32_t at = objs + (int32_t)(i * sizeof(mx_obj_t));
        mx_x64_store16_i(&cc->hot, MX_X64_RBX, at + kind_at, (uint16_t)(arg[1] >> 1));
        mx_jit_store_arg(cc, MX_X64_RBX, at + val_at, arg, MX_X64_RDX);
    }
    mx_x64_mov_rr(&cc->hot, MX_X64_R13, MX_X64_RCX);
    mx_x64_mov_rr(&cc->hot, MX_X64_R14, MX_X64_RAX);
    if (is_spread) {
        int32_t at = objs + (int32_t)(n * sizeof(mx_obj_t));
        mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
        mx_x64_movzx16(&cc->hot, MX_X64_RCX, MX_X64_RAX, kind_at);
        mx_x64_store(&cc->hot, MX_X64_RBX, at + kind_at, MX_X64_RCX);
        mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RAX, val_at);
        mx_x64_store(&cc->hot, MX_X64_RBX, at + val_at, MX_X64_RAX);
    }
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_R13, -2);
    mx_x64_store32(&cc->hot, MX_X64_RBX, mx_jit_disp(frame), MX_X64_RDX);
    mx_x64_store32_i(&cc->hot, MX_X64_RBX, mx_jit_disp(frame) + 4, MX_LINK_C);
    mx_x64_mov_ri(&cc->hot, MX_X64_RDX, (uint64_t)(uintptr_t)rec);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(frame + 1), MX_X64_RDX);
    mx_x64_store16_i(&cc->hot, MX_X64_RBX, mx_jit_disp(frame + 2) + kind_at, MX_KIND_FFI);
    mx_x64_mov_ri(&cc->hot, MX_X64_RDX, ip[6]);
    mx_x64_store(&cc->hot, MX_X64_RBX, mx_jit_disp(frame + 2) + val_at, MX_X64_RDX);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_R13, (int32_t)frame);
    mx_x64_store32(&cc->hot, MX_X64_R14, (int32_t)offsetof(mx_thread_t, frame), MX_X64_RDX);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RDX, (int32_t)(4 + 2 * total));
    mx_x64_store32(&cc->hot, MX_X64_R14, (int32_t)offsetof(mx_thread_t, top), MX_X64_RDX);
    mx_x64_load(&cc->hot, MX_X64_RDX, MX_X64_R12, vals_at);
    mx_x64_store_i32(&cc->hot, MX_X64_RDX, val_at, 0);
    mx_x64_store16_i(&cc->hot, MX_X64_RDX, kind_at, MX_KIND_NIL);
    mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 1);
    mx_x64_alu8_mi(&cc->hot, MX_X64_ALU_OR, MX_X64_R12, pending_at + (int32_t)layout->pending_ok.off, layout->pending_ok.mask);
    mx_x64_alu32_mi8(&cc->hot, MX_X64_ALU_ADD, MX_X64_R12, (int32_t)offsetof(mx_t, no_gc), 1);
    mx_x64_mov_rr(&cc->hot, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_ri(&cc->hot, MX_X64_RSI, total);
    mx_x64_lea(&cc->hot, MX_X64_RDX, MX_X64_RBX, objs);
    mx_val_t bits = { .bits = ip[6] };
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, (uint64_t)(uintptr_t)mx_ref_obj(bits.ref));
    mx_x64_call_r(&cc->hot, MX_X64_RAX);
    mx_jit_drop_all(cc);
    mx_x64_alu32_mi8(&cc->hot, MX_X64_ALU_SUB, MX_X64_R12, (int32_t)offsetof(mx_t, no_gc), 1);
    mx_x64_alu8_mi(&cc->hot, MX_X64_ALU_AND, MX_X64_R12, pending_at + (int32_t)layout->pending_ok.off, (uint8_t)~layout->pending_ok.mask);
    mx_x64_load(&cc->hot, MX_X64_RBX, MX_X64_R14, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_lea_x(&cc->hot, MX_X64_RBX, MX_X64_RBX, MX_X64_R13, 0);
    uint8_t *after = mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI_AFTER);
    mx_x64_test8_mi(&cc->hot, MX_X64_R12, pending_at + (int32_t)layout->pending_set.off, layout->pending_set.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, after);
    mx_x64_test8_mi(&cc->hot, MX_X64_R12, (int32_t)layout->vm_hooked.off, layout->vm_hooked.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, after);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RBX, objs + kind_at, MX_KIND_ERROR);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, after);
    return mx_jit_call_ffi_result(cc, rec, objs, after);
}

uint32_t mx_jit_get_var(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    const mx_resume_t *rec = (const mx_resume_t *)ip[6];
    uint32_t out = (uint32_t)ip[1];
    bool is_table_reg = (ip[3] & 1) == 0 && (mx_kind_t)(ip[3] >> 1) == cc->vm->kinds.table;
    mx_kind_t key_kind = (mx_kind_t)(ip[5] >> 1);
    bool is_str = key_kind == MX_KIND_STRING;
    bool is_ref = key_kind == MX_KIND_CLOSURE || key_kind >= MX_KIND_FIRST_DATA;
    mx_jit_way_t way = mx_jit_value_way(rec);
    bool has_nil = mx_run_cache_find(&rec->cache, (uintptr_t)2 << 48) != NULL;
    if (!is_table_reg || (ip[5] & 1) != 0 || (!is_str && !is_ref) || rec->has_cell_out || (way.code == NULL && !has_nil)) {
        mx_x64_jmp(&cc->hot, mx_jit_get_stub(cc, MX_HOP_TABLE_GET));
        return 0;
    }
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)ip[2]);
    mx_x64_reg_t k = mx_jit_use_g(cc, (uint32_t)ip[4]);
    uint8_t *stub = mx_jit_get_stub(cc, MX_HOP_TABLE_GET);
    mx_jit_miss_t miss = mx_jit_miss_seq(cc, rec, out, t, k, key_kind, 0, 0, stub);
    if (way.code == NULL) {
        mx_x64_patch(miss.found, stub);
        mx_x64_jmp(&cc->hot, miss.start);
        return 0;
    } else if (is_ref) {
        uintptr_t *ref_target = mx_jit_cell(cc, way.code);
        mx_x64_jmp(&cc->hot, miss.start);
        mx_x64_patch(miss.found, cc->hot.p);
        mx_jit_kind_ways(cc, rec, out, way.kind, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind), (int32_t)offsetof(mx_table_entry_t, val), stub, 0);
        return mx_jit_finish_get(cc, rec, out, ref_target, way.kind, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val));
    }
    int32_t size_at = (int32_t)layout->table_size.off;
    mx_x64_test8_mi(&cc->hot, t, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_x64_test8_mi(&cc->hot, k, (int32_t)layout->str_rope.off, layout->str_rope.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_load32(&cc->hot, MX_X64_RAX, k, (int32_t)offsetof(mx_str_t, hash));
    mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_x64_movzx8(&cc->hot, MX_X64_RCX, t, size_at);
    mx_x64_and32_ri(&cc->hot, MX_X64_RCX, layout->table_size.mask);
    mx_x64_mov_ri(&cc->hot, MX_X64_RDX, MX_TABLE_HASH_MIN_CAP);
    mx_x64_shl32_cl(&cc->hot, MX_X64_RDX);
    mx_x64_lea32(&cc->hot, MX_X64_RDX, MX_X64_RDX, -1);
    mx_x64_alu_rr(&cc->hot, MX_X64_ALU_AND, MX_X64_RDX, MX_X64_RAX);
    mx_x64_mov_rr(&cc->hot, MX_X64_RCX, MX_X64_RDX);
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RDX, 1);
    mx_x64_alu_rr(&cc->hot, MX_X64_ALU_ADD, MX_X64_RDX, MX_X64_RCX);
    mx_x64_load(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, entries));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, miss.start);
    mx_x64_lea_x(&cc->hot, MX_X64_RCX, MX_X64_RCX, MX_X64_RDX, 0);
    uintptr_t *target = mx_jit_cell(cc, way.code);
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key_kind), MX_KIND_STRING);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss.start);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, key), k);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, miss.start);
    mx_x64_patch(miss.found, cc->hot.p);
    mx_jit_kind_ways(cc, rec, out, way.kind, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val_kind), (int32_t)offsetof(mx_table_entry_t, val), stub, 0);
    return mx_jit_finish_get(cc, rec, out, target, way.kind, MX_X64_RCX, (int32_t)offsetof(mx_table_entry_t, val));
}

uint32_t mx_jit_goto_side(mx_jit_cc_t *cc, uintptr_t *word, mx_ir_block_t *block) {
    uintptr_t *code = (uintptr_t *)*word;
    if (mx_jit_can_stitch(cc, code)) {
        return mx_jit_continue(cc, code);
    }
    const uint64_t *live = mx_jit_live(cc, block);
    mx_jit_flush_live(cc, &cc->hot, UINT32_MAX, live, live);
    mx_jit_edge_jmp(cc, word, 2 << 1);
    return 0;
}

uint32_t mx_jit_br_eq_str(mx_jit_cc_t *cc) {
    const mx_jit_bit_t *interned = &cc->vm->jit.layout.str_interned;
    const mx_jit_cache_t *ca = mx_jit_const_of(cc, (uint32_t)cc->ip[5]);
    const mx_jit_cache_t *cb = mx_jit_const_of(cc, (uint32_t)cc->ip[6]);
    if (ca != NULL && cb != NULL) {
        const mx_str_t *sa = (const mx_str_t *)(uintptr_t)ca->bits;
        const mx_str_t *sb = (const mx_str_t *)(uintptr_t)cb->bits;
        if (sa == sb) {
            return mx_jit_goto_side(cc, &cc->ip[3], (mx_ir_block_t *)cc->ip[1]);
        } else if (sa->is_interned && sb->is_interned) {
            return mx_jit_goto_side(cc, &cc->ip[4], (mx_ir_block_t *)cc->ip[2]);
        }
    }
    uintptr_t *taken = (uintptr_t *)cc->ip[3];
    uintptr_t *other = (uintptr_t *)cc->ip[4];
    bool can_taken = mx_jit_can_stitch(cc, taken);
    bool can_other = mx_jit_can_stitch(cc, other);
    mx_x64_reg_t a = mx_jit_use_g(cc, (uint32_t)cc->ip[5]);
    mx_x64_reg_t b = mx_jit_use_g(cc, (uint32_t)cc->ip[6]);
    if (!can_taken && !can_other) {
        mx_jit_flush_plan(cc, MX_JIT_PLAN_NONE);
    }
    uint8_t *slow = cc->cold.p;
    mx_jit_spill(cc, &cc->cold);
    mx_x64_mov_rr(&cc->cold, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_rr(&cc->cold, MX_X64_RSI, MX_X64_RBX);
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(uintptr_t)cc->ip);
    mx_x64_call_rip(&cc->cold, &cc->vm->jit.slots[MX_JIT_SLOT_EQ_STR]);
    for (uint32_t i = 0; i < 16; i++) {
        if (cc->g[i].is_used && mx_jit_is_caller_saved((mx_x64_reg_t)i)) {
            mx_x64_load(&cc->cold, (mx_x64_reg_t)i, MX_X64_RBX, mx_jit_home_disp(cc, cc->g[i].reg));
        }
    }
    for (uint32_t i = 0; i < MX_JIT_NXMM; i++) {
        if (cc->x[i].is_used) {
            mx_x64_movsd_load(&cc->cold, i, MX_X64_RBX, mx_jit_home_disp(cc, cc->x[i].reg));
        }
    }
    mx_x64_test8_al(&cc->cold);
    uint8_t *yes = mx_jit_label(&cc->cold, MX_X64_CC_NE);
    uint8_t *no = mx_jit_label_jmp(&cc->cold);
    mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, a, b);
    uint8_t *same = mx_jit_label(&cc->hot, MX_X64_CC_E);
    mx_x64_test8_mi(&cc->hot, a, (int32_t)interned->off, interned->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, slow);
    mx_x64_test8_mi(&cc->hot, b, (int32_t)interned->off, interned->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, slow);
    mx_x64_patch(no, cc->hot.p);
    if (can_other) {
        uint8_t *site = mx_jit_label_jmp(&cc->hot);
        mx_jit_push_pend(cc, other, site);
    } else if (can_taken) {
        mx_x64_jmp(&cc->hot, mx_jit_side(cc, &cc->ip[4], (mx_ir_block_t *)cc->ip[2]));
    } else {
        mx_jit_edge_jmp(cc, &cc->ip[4], 2 << 1);
    }
    mx_x64_patch(same, cc->hot.p);
    mx_x64_patch(yes, cc->hot.p);
    if (can_taken) {
        return mx_jit_continue(cc, taken);
    } else if (can_other) {
        mx_x64_jmp(&cc->hot, mx_jit_side(cc, &cc->ip[3], (mx_ir_block_t *)cc->ip[1]));
    } else {
        mx_jit_edge_jmp(cc, &cc->ip[3], 2 << 1);
    }
    return 0;
}

uint32_t mx_jit_br_eq_f(mx_jit_cc_t *cc, uint32_t left, uint32_t right) {
    uintptr_t *taken = (uintptr_t *)cc->ip[3];
    uintptr_t *other = (uintptr_t *)cc->ip[4];
    bool can_taken = mx_jit_can_stitch(cc, taken);
    bool can_other = mx_jit_can_stitch(cc, other);
    if (!can_taken && !can_other) {
        mx_jit_flush_plan(cc, MX_JIT_PLAN_NONE);
    }
    uint8_t *away;
    if (can_other) {
        away = cc->cold.p;
        uint8_t *site = mx_jit_label_jmp(&cc->cold);
        mx_jit_push_pend(cc, other, site);
    } else if (can_taken) {
        away = mx_jit_side(cc, &cc->ip[4], (mx_ir_block_t *)cc->ip[2]);
    } else {
        away = NULL;
    }
    mx_x64_ucomisd_rr(&cc->hot, left, right);
    if (away != NULL) {
        mx_x64_jcc(&cc->hot, MX_X64_CC_P, away);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, away);
    } else {
        mx_jit_edge(cc, MX_X64_CC_P, &cc->ip[4], 2 << 1);
        mx_jit_edge(cc, MX_X64_CC_NE, &cc->ip[4], 2 << 1);
    }
    if (can_taken) {
        return mx_jit_continue(cc, taken);
    } else if (can_other) {
        mx_x64_jmp(&cc->hot, mx_jit_side(cc, &cc->ip[3], (mx_ir_block_t *)cc->ip[1]));
    } else {
        mx_jit_edge_jmp(cc, &cc->ip[3], 2 << 1);
    }
    return 0;
}

void mx_jit_upval_set(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    if (cc->rb->block->func->has_cells) {
        mx_jit_hop_call(cc, MX_HOP_UPVAL_SET);
        mx_jit_drop_all(cc);
        return;
    }
    const mx_jit_bit_t *barrier = &cc->vm->jit.layout.head_barrier;
    const uintptr_t *arg = ip + 2;
    mx_kind_t kind = (mx_kind_t)(arg[1] >> 1);
    mx_x64_reg_t cl = mx_jit_use_g(cc, 0);
    uint8_t *stub = mx_jit_slow_stub(cc, MX_HOP_UPVAL_SET);
    mx_x64_load(&cc->hot, MX_X64_RAX, cl, (int32_t)(offsetof(mx_obj_closure_t, upvals) + ip[1] * sizeof(mx_obj_upval_t *)));
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, thread));
    mx_x64_test_rr(&cc->hot, MX_X64_RCX, MX_X64_RCX);
    uint8_t *open = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_x64_test8_mi(&cc->hot, MX_X64_RAX, (int32_t)barrier->off, barrier->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_store16_i(&cc->hot, MX_X64_RAX, (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, kind)), kind);
    mx_jit_store_arg(cc, MX_X64_RAX, (int32_t)(offsetof(mx_obj_upval_t, value) + offsetof(mx_obj_t, val)), arg, MX_X64_RCX);
    uint8_t *done = mx_jit_label_jmp(&cc->hot);
    mx_x64_patch(open, cc->hot.p);
    mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RCX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_load32(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, kind_at));
    mx_x64_lea_x(&cc->hot, MX_X64_RDX, MX_X64_RCX, MX_X64_RDX, 0);
    mx_x64_store_i32(&cc->hot, MX_X64_RDX, 0, kind);
    mx_x64_load32(&cc->hot, MX_X64_RDX, MX_X64_RAX, (int32_t)offsetof(mx_obj_upval_t, at));
    mx_x64_lea_x(&cc->hot, MX_X64_RDX, MX_X64_RCX, MX_X64_RDX, 0);
    mx_jit_store_arg(cc, MX_X64_RDX, 0, arg, MX_X64_RAX);
    mx_x64_patch(done, cc->hot.p);
    mx_jit_slow_join(cc);
}

void mx_jit_call_fast_rt(mx_jit_cc_t *cc) {
    mx_jit_flush(cc);
    mx_jit_call_rt(cc, &cc->hot, MX_JIT_SLOT_CALL_FAST, MX_HOP_CALL_FAST);
    mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI));
    mx_x64_jmp_m(&cc->hot, MX_X64_RAX, 8);
}

uint8_t *mx_jit_intrinsic_stub(mx_jit_cc_t *cc) {
    uint8_t *stub = cc->cold.p;
    mx_jit_spill(cc, &cc->cold);
    mx_jit_call_rt(cc, &cc->cold, MX_JIT_SLOT_CALL_FAST, MX_HOP_CALL_FAST);
    mx_x64_test_rr(&cc->cold, MX_X64_RAX, MX_X64_RAX);
    uint8_t *none = mx_jit_label(&cc->cold, MX_X64_CC_E);
    mx_x64_jmp_m(&cc->cold, MX_X64_RAX, 8);
    mx_x64_patch(none, cc->cold.p);
    mx_jit_frame_seq(cc, &cc->cold, MX_JIT_SLOT_FFI);
    return stub;
}

uint32_t mx_jit_goto_code(mx_jit_cc_t *cc, const mx_resume_t *rec, uintptr_t *code) {
    if (mx_jit_can_stitch(cc, code)) {
        return mx_jit_continue(cc, code);
    }
    const uint64_t *live = mx_jit_live(cc, rec->target);
    mx_jit_flush_live(cc, &cc->hot, UINT32_MAX, live, live);
    mx_jit_edge_jmp(cc, mx_jit_cell(cc, code), 0);
    return 0;
}

uint32_t mx_jit_intrinsic_done(mx_jit_cc_t *cc, const mx_resume_t *rec, mx_kind_t kind) {
    if (rec->want == MX_IR_WANT_MULTI) {
        int32_t vals_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals));
        int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
        const uintptr_t arg[2] = { cc->frame, 0 };
        mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
        mx_x64_store16_i(&cc->hot, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, kind), kind);
        mx_jit_store_arg(cc, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val), arg, MX_X64_RCX);
        mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 1);
        mx_jit_drop(cc, cc->frame);
        uintptr_t mkey = 1 | MX_RUN_KEY_RESUMED;
        uintptr_t *mcode = mx_run_cache_find(&rec->cache, mkey);
        if (mcode == NULL) {
            mcode = mx_version_values(cc->vm, rec, NULL, 0, true)->code;
            mx_run_cache_put(cc->vm, (mx_resume_cache_t *)&rec->cache, mkey, (uintptr_t)mcode);
        }
        return mx_jit_goto_code(cc, rec, mcode);
    }
    uintptr_t key = ((uintptr_t)2 << 48) | kind;
    uintptr_t *code = mx_run_cache_find(&rec->cache, key);
    if (code == NULL) {
        code = mx_version_values(cc->vm, rec, &kind, 1, false)->code;
        mx_run_cache_put(cc->vm, (mx_resume_cache_t *)&rec->cache, key, (uintptr_t)code);
    }
    return mx_jit_goto_code(cc, rec, code);
}

void mx_jit_move_arg(mx_jit_cc_t *cc, uint32_t out, const uintptr_t *arg) {
    if ((arg[1] & 1) != 0) {
        mx_jit_move_i(cc, out, arg[0]);
    } else {
        mx_jit_move_r(cc, out, (uint32_t)arg[0]);
    }
}

mx_jit_way_t mx_jit_inext_way(const mx_resume_t *rec) {
    uintptr_t key = rec->cache.key;
    uintptr_t code = rec->cache.code;
    for (uint32_t i = 0; i < MX_RESUME_WAYS; i++) {
        uintptr_t flags = key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED);
        mx_kind_t k0 = (mx_kind_t)key;
        mx_kind_t k1 = (mx_kind_t)(key >> 16);
        if (code != 0 && mx_jit_key_len(key) == 3 && flags == MX_RUN_KEY_RESUMED && k0 == MX_KIND_INT && k1 != MX_KIND_NIL) {
            return (mx_jit_way_t) { .code = (uintptr_t *)code, .kind = k1 };
        } else if (i + 1 < MX_RESUME_WAYS && rec->cache.more != NULL) {
            key = rec->cache.more[2 * i];
            code = rec->cache.more[2 * i + 1];
        } else {
            code = 0;
        }
    }
    return (mx_jit_way_t) { .code = NULL, .kind = MX_KIND_NIL };
}

uint32_t mx_jit_inext(mx_jit_cc_t *cc, mx_jit_way_t way) {
    uintptr_t *ip = cc->ip;
    const mx_jit_layout_t *layout = &cc->vm->jit.layout;
    const mx_resume_t *rec = (const mx_resume_t *)ip[5];
    const uintptr_t *a = ip + 8;
    const uintptr_t *b = ip + 10;
    mx_x64_reg_t fn = 0;
    if ((ip[2] & 1) == 0) {
        fn = mx_jit_use_g(cc, (uint32_t)ip[1]);
    }
    mx_x64_reg_t t = mx_jit_use_g(cc, (uint32_t)a[0]);
    mx_x64_reg_t i = mx_jit_use_g(cc, (uint32_t)b[0]);
    uint8_t *stub = mx_jit_intrinsic_stub(cc);
    if ((ip[2] & 1) == 0) {
        mx_x64_mov_ri(&cc->hot, MX_X64_RCX, ip[6]);
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, fn, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
    mx_x64_test8_mi(&cc->hot, t, (int32_t)layout->table_plain.off, layout->table_plain.mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
    mx_x64_cmp_mi(&cc->hot, t, (int32_t)offsetof(mx_obj_table_t, meta), 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    mx_x64_load32(&cc->hot, MX_X64_RCX, t, (int32_t)offsetof(mx_obj_table_t, array_len));
    mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, i, MX_X64_RCX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_AE, stub);
    mx_x64_mov_rr(&cc->hot, MX_X64_RAX, i);
    mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 4);
    mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, t, (int32_t)offsetof(mx_obj_table_t, array));
    mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, kind), way.kind);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    if (way.kind == MX_KIND_FLOAT) {
        uint32_t d = mx_jit_def_x(cc, rec->out + 1);
        mx_x64_movsd_load(&cc->hot, d, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val));
    } else {
        mx_x64_reg_t d = mx_jit_def_g(cc, rec->out + 1);
        mx_x64_load(&cc->hot, d, MX_X64_RAX, (int32_t)offsetof(mx_obj_t, val));
    }
    mx_x64_reg_t next = mx_jit_def_g(cc, rec->out);
    mx_x64_lea(&cc->hot, next, i, 1);
    return mx_jit_goto_code(cc, rec, way.code);
}

uint32_t mx_jit_intrinsic(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_resume_t *rec = (const mx_resume_t *)ip[5];
    uint32_t n = (uint32_t)ip[3];
    uintptr_t fast = ip[7];
    const uintptr_t *a = ip + 8;
    const uintptr_t *b = ip + 10;
    const uintptr_t *c = ip + 12;
    bool is_lit = (ip[2] & 1) != 0;
    if (ip[4] != 0 || (mx_kind_t)(ip[2] >> 1) != MX_KIND_FFI || (is_lit && ip[1] != ip[6]) || rec->kind != MX_RESUME_VALUES || rec->has_cell_out || n == 0) {
        return UINT32_MAX;
    }
    if (fast == (uintptr_t)mx_ffi_base_inext_fast && rec->want == 2 && n == 2 && (a[1] & 1) == 0 && (b[1] & 1) == 0 && (mx_kind_t)(a[1] >> 1) == cc->vm->kinds.table && (mx_kind_t)(b[1] >> 1) == MX_KIND_INT) {
        mx_jit_way_t way = mx_jit_inext_way(rec);
        if (way.code != NULL) {
            return mx_jit_inext(cc, way);
        }
    }
    if (rec->want != 1 && rec->want != MX_IR_WANT_MULTI) {
        return UINT32_MAX;
    }
    mx_kind_t ka = (mx_kind_t)(a[1] >> 1);
    bool a_reg = (a[1] & 1) == 0;
    uint32_t out;
    if (rec->want == MX_IR_WANT_MULTI) {
        out = cc->frame;
    } else {
        out = rec->out;
    }
    uint32_t ra = (uint32_t)a[0];
    bool is_type = fast == (uintptr_t)mx_ffi_base_type_fast;
    bool is_sqrt = fast == (uintptr_t)mx_ffi_math_sqrt_fast && a_reg && mx_kind_is_number(ka);
    bool is_abs = fast == (uintptr_t)mx_ffi_math_abs_fast && a_reg && mx_kind_is_number(ka);
    bool is_floor = fast == (uintptr_t)mx_ffi_math_floor_fast && a_reg && mx_kind_is_number(ka);
    bool is_ceil = fast == (uintptr_t)mx_ffi_math_ceil_fast && a_reg && mx_kind_is_number(ka);
    bool is_len = fast == (uintptr_t)mx_ffi_string_len_fast && a_reg && ka == MX_KIND_STRING;
    bool is_byte = fast == (uintptr_t)mx_ffi_string_byte_fast && a_reg && ka == MX_KIND_STRING && n <= 2 && (n == 1 || (mx_kind_t)(b[1] >> 1) == MX_KIND_INT);
    bool is_max = fast == (uintptr_t)mx_ffi_math_max_fast;
    bool is_min = fast == (uintptr_t)mx_ffi_math_min_fast;
    bool is_pick = (is_max || is_min) && n == 2 && a_reg && (b[1] & 1) == 0 && (mx_kind_t)(b[1] >> 1) == ka && mx_kind_is_number(ka);
    bool is_str_id = fast == (uintptr_t)mx_ffi_base_tostring_fast && a_reg && ka == MX_KIND_STRING;
    bool is_sub = fast == (uintptr_t)mx_ffi_string_sub_fast && a_reg && ka == MX_KIND_STRING && n == 3 && (mx_kind_t)(b[1] >> 1) == MX_KIND_INT && b[0] == c[0] && b[1] == c[1];
    bool is_init = fast == (uintptr_t)mx_prim_forprep_initial_fast && n == 2 && ((ka == MX_KIND_INT && (mx_kind_t)(b[1] >> 1) == MX_KIND_INT) || (ka == MX_KIND_FLOAT && mx_kind_is_number((mx_kind_t)(b[1] >> 1))));
    bool is_limit = fast == (uintptr_t)mx_prim_forprep_limit_fast && n == 3 && ka == MX_KIND_INT && (mx_kind_t)(b[1] >> 1) == MX_KIND_INT && (mx_kind_t)(c[1] >> 1) == MX_KIND_INT && (c[1] & 1) != 0 && (c[0] == 1 || c[0] == UINT64_MAX);
    if (!is_type && !is_sqrt && !is_abs && !is_floor && !is_ceil && !is_len && !is_byte && !is_pick && !is_str_id && !is_sub && !is_init && !is_limit) {
        return UINT32_MAX;
    }
    mx_x64_reg_t fn = 0;
    if (!is_lit) {
        fn = mx_jit_use_g(cc, (uint32_t)ip[1]);
    }
    if (is_byte && n == 2 && (b[1] & 1) == 0) {
        mx_jit_use_g(cc, (uint32_t)b[0]);
    }
    if (is_sub && (b[1] & 1) == 0) {
        mx_jit_use_g(cc, (uint32_t)b[0]);
    }
    if (is_pick && ka == MX_KIND_INT) {
        mx_jit_use_g(cc, (uint32_t)b[0]);
    }
    if (is_pick && ka == MX_KIND_FLOAT) {
        mx_jit_use_x(cc, ra);
        mx_jit_use_x(cc, (uint32_t)b[0]);
    } else if ((is_sqrt || is_floor || is_ceil) && ka == MX_KIND_FLOAT) {
        mx_jit_use_x(cc, ra);
    } else if (!is_type && !is_init && !is_limit && ka != MX_KIND_FLOAT) {
        mx_jit_use_g(cc, ra);
    }
    uint8_t *stub = NULL;
    if (!is_lit || (!is_init && !is_limit && !is_type)) {
        stub = mx_jit_intrinsic_stub(cc);
    }
    if (!is_lit) {
        mx_x64_mov_ri(&cc->hot, MX_X64_RCX, ip[6]);
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, fn, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
    }
    if (is_init || is_limit) {
        mx_jit_move_arg(cc, out, a);
        return mx_jit_intrinsic_done(cc, rec, ka);
    } else if (is_str_id) {
        mx_jit_move_r(cc, out, ra);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_STRING);
    } else if (is_sub) {
        const mx_jit_bit_t *rope = &cc->vm->jit.layout.str_rope;
        mx_x64_reg_t s = mx_jit_use_g(cc, ra);
        mx_x64_test8_mi(&cc->hot, s, (int32_t)rope->off, rope->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        if ((b[1] & 1) != 0) {
            mx_x64_mov_ri(&cc->hot, MX_X64_RAX, b[0] - 1);
        } else {
            mx_x64_lea(&cc->hot, MX_X64_RAX, mx_jit_use_g(cc, (uint32_t)b[0]), -1);
        }
        mx_x64_load32(&cc->hot, MX_X64_RCX, s, (int32_t)offsetof(mx_str_t, len));
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_AE, stub);
        mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, s, (int32_t)offsetof(mx_str_t, buf));
        mx_x64_movzx8(&cc->hot, MX_X64_RAX, MX_X64_RAX, 0);
        mx_x64_shift_i(&cc->hot, MX_X64_SHIFT_SHL, MX_X64_RAX, 4);
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, MX_X64_R12);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RAX, (int32_t)(offsetof(mx_t, char_strs) + offsetof(mx_obj_t, kind)), MX_KIND_NIL);
        mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_load(&cc->hot, d, MX_X64_RAX, (int32_t)(offsetof(mx_t, char_strs) + offsetof(mx_obj_t, val)));
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_STRING);
    } else if (is_type) {
        mx_obj_t probe = mx_obj_of_raw(ka, (mx_val_t) { .bits = 0 });
        mx_obj_t name = mx_lua_of(cc->vm)->type_names[mx_meta_lua_type(cc->vm, probe)];
        mx_jit_move_i(cc, out, name.val.bits);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_STRING);
    } else if (is_sqrt) {
        if (ka == MX_KIND_FLOAT) {
            uint32_t s = mx_jit_use_x(cc, ra);
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_sqrtsd_rr(&cc->hot, d, s);
        } else {
            mx_jit_to_f64(cc, MX_JIT_XMM_SCRATCH, ra);
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_sqrtsd_rr(&cc->hot, d, MX_JIT_XMM_SCRATCH);
        }
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_FLOAT);
    } else if (is_abs && ka == MX_KIND_FLOAT) {
        mx_jit_load_reg(cc, MX_X64_RAX, ra);
        mx_x64_btr_i(&cc->hot, MX_X64_RAX, 63);
        uint32_t d = mx_jit_def_x(cc, out);
        mx_x64_movq_xr(&cc->hot, d, MX_X64_RAX);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_FLOAT);
    } else if (is_abs) {
        mx_x64_reg_t s = mx_jit_use_g(cc, ra);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_mov_rr(&cc->hot, d, s);
        mx_x64_test_rr(&cc->hot, d, d);
        uint8_t *pos = mx_jit_label(&cc->hot, MX_X64_CC_GE);
        mx_x64_neg(&cc->hot, d);
        mx_x64_patch(pos, cc->hot.p);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else if ((is_floor || is_ceil) && ka == MX_KIND_INT) {
        mx_x64_reg_t s = mx_jit_use_g(cc, ra);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_mov_rr(&cc->hot, d, s);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else if (is_floor || is_ceil) {
        uint32_t s = mx_jit_use_x(cc, ra);
        mx_x64_cvttsd2si(&cc->hot, MX_X64_RAX, s);
        mx_x64_mov_ri(&cc->hot, MX_X64_RCX, (uint64_t)INT64_MIN);
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
        mx_x64_xorpd(&cc->hot, MX_JIT_XMM_SCRATCH, MX_JIT_XMM_SCRATCH);
        mx_x64_cvtsi2sd_rr(&cc->hot, MX_JIT_XMM_SCRATCH, MX_X64_RAX);
        if (is_floor) {
            mx_x64_ucomisd_rr(&cc->hot, MX_JIT_XMM_SCRATCH, s);
            uint8_t *exact = mx_jit_label(&cc->hot, MX_X64_CC_BE);
            mx_x64_alu_ri(&cc->hot, MX_X64_ALU_SUB, MX_X64_RAX, 1);
            mx_x64_patch(exact, cc->hot.p);
        } else {
            mx_x64_ucomisd_rr(&cc->hot, s, MX_JIT_XMM_SCRATCH);
            uint8_t *exact = mx_jit_label(&cc->hot, MX_X64_CC_BE);
            mx_x64_alu_ri(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, 1);
            mx_x64_patch(exact, cc->hot.p);
        }
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_mov_rr(&cc->hot, d, MX_X64_RAX);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else if (is_len) {
        mx_x64_reg_t s = mx_jit_use_g(cc, ra);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_load32(&cc->hot, d, s, (int32_t)offsetof(mx_str_t, len));
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else if (is_byte) {
        const mx_jit_bit_t *rope = &cc->vm->jit.layout.str_rope;
        mx_x64_reg_t s = mx_jit_use_g(cc, ra);
        mx_x64_test8_mi(&cc->hot, s, (int32_t)rope->off, rope->mask);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        if (n == 1) {
            mx_x64_mov_ri(&cc->hot, MX_X64_RAX, 0);
        } else if ((b[1] & 1) != 0) {
            mx_x64_mov_ri(&cc->hot, MX_X64_RAX, b[0] - 1);
        } else {
            mx_x64_lea(&cc->hot, MX_X64_RAX, mx_jit_use_g(cc, (uint32_t)b[0]), -1);
        }
        mx_x64_load32(&cc->hot, MX_X64_RCX, s, (int32_t)offsetof(mx_str_t, len));
        mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, MX_X64_RAX, MX_X64_RCX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_AE, stub);
        mx_x64_alu_rm(&cc->hot, MX_X64_ALU_ADD, MX_X64_RAX, s, (int32_t)offsetof(mx_str_t, buf));
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_movzx8(&cc->hot, d, MX_X64_RAX, 0);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else if (ka == MX_KIND_INT) {
        mx_x64_reg_t sa = mx_jit_use_g(cc, ra);
        mx_x64_reg_t sb = mx_jit_use_g(cc, (uint32_t)b[0]);
        mx_x64_reg_t d = mx_jit_def_g(cc, out);
        mx_x64_mov_rr(&cc->hot, d, sa);
        if (is_max) {
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, sa, sb);
        } else {
            mx_x64_alu_rr(&cc->hot, MX_X64_ALU_CMP, sb, sa);
        }
        uint8_t *keep = mx_jit_label(&cc->hot, MX_X64_CC_GE);
        mx_x64_mov_rr(&cc->hot, d, sb);
        mx_x64_patch(keep, cc->hot.p);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_INT);
    } else {
        uint32_t sa = mx_jit_use_x(cc, ra);
        uint32_t sb = mx_jit_use_x(cc, (uint32_t)b[0]);
        uint32_t d = mx_jit_def_x(cc, out);
        mx_x64_movaps(&cc->hot, d, sa);
        if (is_max) {
            mx_x64_ucomisd_rr(&cc->hot, sb, sa);
        } else {
            mx_x64_ucomisd_rr(&cc->hot, sa, sb);
        }
        uint8_t *keep = mx_jit_label(&cc->hot, MX_X64_CC_BE);
        mx_x64_movaps(&cc->hot, d, sb);
        mx_x64_patch(keep, cc->hot.p);
        return mx_jit_intrinsic_done(cc, rec, MX_KIND_FLOAT);
    }
}

uint32_t mx_jit_call_fast(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    const mx_resume_t *rec = (const mx_resume_t *)ip[5];
    uint32_t n = (uint32_t)ip[3];
    mx_kind_t fn_kind = (mx_kind_t)(ip[2] >> 1);
    bool is_lit = (ip[2] & 1) != 0;
    uint32_t intrinsic = mx_jit_intrinsic(cc);
    if (intrinsic != UINT32_MAX) {
        return intrinsic;
    }
    bool is_spread = ip[4] != 0;
    uint32_t total = n + (uint32_t)is_spread;
    if (total > MX_RUN_SMALL || fn_kind != MX_KIND_FFI || ip[7] == 0 || rec->kind != MX_RESUME_VALUES || (rec->want > MX_JIT_WANT_MAX && rec->want != MX_IR_WANT_MULTI)) {
        mx_jit_call_fast_rt(cc);
        return 0;
    }
    if (is_lit && ip[1] != ip[6]) {
        mx_jit_stop(cc);
        return 0;
    }
    mx_jit_flush(cc);
    int32_t kind_at = (int32_t)offsetof(mx_obj_t, kind);
    int32_t val_at = (int32_t)offsetof(mx_obj_t, val);
    if (!is_lit) {
        mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[6]);
        mx_x64_cmp_mr(&cc->hot, MX_X64_RBX, mx_jit_disp((uint32_t)ip[1]), MX_X64_RAX);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI));
    }
    int32_t vals_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, vals));
    int32_t len_at = (int32_t)(offsetof(mx_t, rets) + offsetof(mx_rets_t, len));
    int32_t spread_at = (int32_t)(n * sizeof(mx_obj_t));
    const uintptr_t *a = ip + 8;
    const uintptr_t *b = ip + 10;
    uint8_t *next_ok = NULL;
    if (ip[7] == (uintptr_t)mx_ffi_base_next_fast && n == 2 && !is_spread && (a[1] & 1) == 0 && (mx_kind_t)(a[1] >> 1) == cc->vm->kinds.table) {
        mx_jit_load_arg(cc, MX_X64_RAX, a);
        mx_jit_load_arg(cc, MX_X64_RDX, b);
        mx_x64_mov_ri(&cc->hot, MX_X64_RCX, (uint64_t)(mx_kind_t)(b[1] >> 1));
        mx_x64_call_rip(&cc->hot, &cc->vm->jit.slots[MX_JIT_SLOT_NEXT]);
        mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
        uint8_t *normal = mx_jit_label(&cc->hot, MX_X64_CC_E);
        mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 2);
        next_ok = mx_jit_label_jmp(&cc->hot);
        mx_x64_patch(normal, cc->hot.p);
    }
    if (is_spread) {
        mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, 1);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI));
    }
    for (uint32_t i = 0; i < n; i++) {
        const uintptr_t *arg = ip + 8 + 2 * i;
        int32_t at = (int32_t)(i * sizeof(mx_obj_t));
        mx_x64_store16_i(&cc->hot, MX_X64_RSP, at + kind_at, (uint16_t)(arg[1] >> 1));
        mx_jit_store_arg(cc, MX_X64_RSP, at + val_at, arg, MX_X64_RAX);
    }
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
    if (is_spread) {
        mx_x64_movzx16(&cc->hot, MX_X64_RCX, MX_X64_RAX, kind_at);
        mx_x64_store(&cc->hot, MX_X64_RSP, spread_at + kind_at, MX_X64_RCX);
        mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RAX, val_at);
        mx_x64_store(&cc->hot, MX_X64_RSP, spread_at + val_at, MX_X64_RCX);
    }
    mx_x64_store_i32(&cc->hot, MX_X64_RAX, val_at, 0);
    mx_x64_store16_i(&cc->hot, MX_X64_RAX, kind_at, MX_KIND_NIL);
    mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 1);
    mx_x64_mov_rr(&cc->hot, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_ri(&cc->hot, MX_X64_RSI, total);
    mx_x64_mov_rr(&cc->hot, MX_X64_RDX, MX_X64_RSP);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, ip[7]);
    mx_x64_call_r(&cc->hot, MX_X64_RAX);
    mx_jit_drop_all(cc);
    mx_x64_test8_al(&cc->hot);
    uint8_t *ok = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    if (is_spread) {
        mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, vals_at);
        mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RSP, spread_at + kind_at);
        mx_x64_store(&cc->hot, MX_X64_RAX, kind_at, MX_X64_RCX);
        mx_x64_load(&cc->hot, MX_X64_RCX, MX_X64_RSP, spread_at + val_at);
        mx_x64_store(&cc->hot, MX_X64_RAX, val_at, MX_X64_RCX);
        mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 1);
    } else {
        mx_x64_store32_i(&cc->hot, MX_X64_R12, len_at, 0);
    }
    mx_x64_jmp(&cc->hot, mx_jit_frame_stub(cc, MX_JIT_SLOT_FFI));
    mx_x64_patch(ok, cc->hot.p);
    if (next_ok != NULL) {
        mx_x64_patch(next_ok, cc->hot.p);
    }
    uint8_t *helper = cc->cold.p;
    mx_x64_mov_rr(&cc->cold, MX_X64_RDI, MX_X64_R12);
    mx_x64_mov_rr(&cc->cold, MX_X64_RSI, MX_X64_RBX);
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(uintptr_t)rec);
    mx_x64_mov_rr(&cc->cold, MX_X64_RCX, MX_X64_RSP);
    mx_x64_call_rip(&cc->cold, &cc->vm->jit.slots[MX_JIT_SLOT_DELIVER]);
    mx_x64_jmp_m(&cc->cold, MX_X64_RAX, 8);
    uint8_t *stub = mx_jit_ways_stub(cc, rec, MX_X64_RSP, 0, helper);
    uintptr_t key = rec->cache.key;
    uintptr_t *code = (uintptr_t *)rec->cache.code;
    uint32_t want = rec->want;
    bool is_plain_out = !rec->has_cell_out && want <= 3;
    bool is_one = is_plain_out && want == 1 && code != NULL && (key >> 48) == 2 && (key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED)) == 0 && (mx_kind_t)key != MX_KIND_NIL;
    bool is_many = is_plain_out && want != 1 && code != NULL && mx_jit_key_len(key) == want + 1 && (key & (MX_RUN_KEY_HOOKED | MX_RUN_KEY_RESUMED)) == MX_RUN_KEY_RESUMED;
    if (!is_many && !is_one) {
        mx_x64_jmp(&cc->hot, stub);
        return 0;
    }
    uintptr_t *target = mx_jit_cell(cc, code);
    if (is_one) {
        mx_x64_cmp32_mi(&cc->hot, MX_X64_R12, len_at, 0);
        mx_x64_jcc(&cc->hot, MX_X64_CC_E, stub);
        mx_x64_cmp16_mi(&cc->hot, MX_X64_RSP, kind_at, (uint16_t)key);
        mx_x64_jcc(&cc->hot, MX_X64_CC_NE, stub);
        return mx_jit_finish_get(cc, rec, rec->out, target, (mx_kind_t)key, MX_X64_RSP, val_at);
    }
    return mx_jit_many(cc, rec, key, code, MX_X64_RSP, 0, stub);
}

void mx_jit_return_tail(mx_jit_cc_t *cc) {
    mx_x64_load32(&cc->hot, MX_X64_RCX, MX_X64_RBX, -16);
    mx_x64_load(&cc->hot, MX_X64_RSI, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_load(&cc->hot, MX_X64_RSI, MX_X64_RSI, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_lea_x(&cc->hot, MX_X64_RBX, MX_X64_RSI, MX_X64_RCX, 16);
}

void mx_jit_return_head(mx_jit_cc_t *cc, uint8_t *exit) {
    const mx_jit_bit_t *cell = &cc->vm->jit.layout.rec_cell_out;
    mx_x64_cmp8_mi(&cc->hot, MX_X64_RBX, -16 + 4, MX_LINK_LUA);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    mx_x64_load(&cc->hot, MX_X64_RDX, MX_X64_RBX, -8);
    mx_x64_test8_mi(&cc->hot, MX_X64_RDX, (int32_t)cell->off, cell->mask);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
}

void mx_jit_return_enter(mx_jit_cc_t *cc) {
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RDX, (int32_t)(offsetof(mx_resume_t, cache) + offsetof(mx_resume_cache_t, code)));
    mx_x64_jmp_m(&cc->hot, MX_X64_RAX, 8);
}

void mx_jit_return1(mx_jit_cc_t *cc) {
    const uintptr_t *arg = cc->ip + 1;
    uint8_t *exit = mx_jit_return_stub(cc, MX_HOP_RETURN1);
    int32_t key_at = (int32_t)(offsetof(mx_resume_t, cache) + offsetof(mx_resume_cache_t, key));
    mx_kind_t kind = (mx_kind_t)(arg[1] >> 1);
    uint64_t key1 = ((uint64_t)2 << 48) | MX_RUN_KEY_RESUMED | kind;
    uint64_t key0 = ((uint64_t)1 << 48) | MX_RUN_KEY_RESUMED;
    mx_jit_return_head(cc, exit);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key1);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RDX, key_at, MX_X64_RAX);
    uint8_t *other = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_jit_load_arg(cc, MX_X64_RAX, arg);
    mx_jit_return_tail(cc);
    mx_x64_movzx16(&cc->hot, MX_X64_RCX, MX_X64_RDX, (int32_t)offsetof(mx_resume_t, out));
    mx_x64_store_x(&cc->hot, MX_X64_RBX, MX_X64_RCX, 0, MX_X64_RAX);
    mx_jit_return_enter(cc);
    mx_x64_patch(other, cc->hot.p);
    mx_x64_call_rip(&cc->hot, &cc->vm->jit.slots[MX_JIT_SLOT_RET_WAYS]);
    mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
    uint8_t *drop = mx_jit_label(&cc->hot, MX_X64_CC_E);
    mx_x64_push(&cc->hot, MX_X64_RAX);
    mx_jit_load_arg(cc, MX_X64_RAX, arg);
    mx_jit_return_tail(cc);
    mx_x64_movzx16(&cc->hot, MX_X64_RCX, MX_X64_RDX, (int32_t)offsetof(mx_resume_t, out));
    mx_x64_store_x(&cc->hot, MX_X64_RBX, MX_X64_RCX, 0, MX_X64_RAX);
    mx_x64_pop(&cc->hot, MX_X64_RAX);
    mx_x64_jmp_m(&cc->hot, MX_X64_RAX, 8);
    mx_x64_patch(drop, cc->hot.p);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key0);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RDX, key_at, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    mx_jit_return_tail(cc);
    mx_jit_return_enter(cc);
}

uint8_t *mx_jit_return_stub(mx_jit_cc_t *cc, mx_hop_t hop) {
    uint8_t *stub = cc->cold.p;
    mx_jit_spill(cc, &cc->cold);
    mx_x64_mov_ri(&cc->cold, MX_X64_RDX, (uint64_t)(uintptr_t)cc->ip);
    mx_x64_mov_ri(&cc->cold, MX_X64_RCX, hop);
    mx_x64_mov_ri(&cc->cold, MX_X64_R8, (uint64_t)(uintptr_t)cc->rb);
    mx_x64_jmp(&cc->cold, cc->vm->jit.slots[MX_JIT_SLOT_T_RETURN]);
    return stub;
}

void mx_jit_return_n(mx_jit_cc_t *cc) {
    uintptr_t *ip = cc->ip;
    uint32_t n = (uint32_t)ip[1];
    uint8_t *exit = mx_jit_return_stub(cc, MX_HOP_RETURN);
    if (ip[2] != 0 || n < 2 || n > 3) {
        mx_x64_jmp(&cc->hot, exit);
        return;
    }
    uint64_t key = ((uint64_t)(n + 1) << 48) | MX_RUN_KEY_RESUMED;
    for (uint32_t i = 0; i < n; i++) {
        key |= (uint64_t)(mx_kind_t)(ip[3 + 2 * i + 1] >> 1) << (16 * i);
    }
    mx_jit_return_head(cc, exit);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RDX, (int32_t)(offsetof(mx_resume_t, cache) + offsetof(mx_resume_cache_t, key)), MX_X64_RAX);
    uint8_t *other = mx_jit_label(&cc->hot, MX_X64_CC_NE);
    mx_jit_return_store(cc, n);
    mx_jit_return_tail(cc);
    mx_jit_return_enter(cc);
    mx_x64_patch(other, cc->hot.p);
    mx_x64_call_rip(&cc->hot, &cc->vm->jit.slots[MX_JIT_SLOT_RET_WAYS]);
    mx_x64_test_rr(&cc->hot, MX_X64_RAX, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_E, exit);
    mx_x64_push(&cc->hot, MX_X64_RAX);
    mx_jit_return_store(cc, n);
    mx_jit_return_tail(cc);
    mx_x64_pop(&cc->hot, MX_X64_RAX);
    mx_x64_jmp_m(&cc->hot, MX_X64_RAX, 8);
}

void mx_jit_return_store(mx_jit_cc_t *cc, uint32_t n) {
    uintptr_t *ip = cc->ip;
    mx_x64_load32(&cc->hot, MX_X64_RCX, MX_X64_RBX, -16);
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_R12, (int32_t)offsetof(mx_t, thread));
    mx_x64_load(&cc->hot, MX_X64_RAX, MX_X64_RAX, (int32_t)offsetof(mx_thread_t, slots));
    mx_x64_lea_x(&cc->hot, MX_X64_RCX, MX_X64_RAX, MX_X64_RCX, 16);
    mx_x64_movzx16(&cc->hot, MX_X64_RAX, MX_X64_RDX, (int32_t)offsetof(mx_resume_t, out));
    mx_x64_lea_x(&cc->hot, MX_X64_RCX, MX_X64_RCX, MX_X64_RAX, 0);
    for (uint32_t i = 0; i < n; i++) {
        mx_jit_store_arg(cc, MX_X64_RCX, mx_jit_disp(i), ip + 3 + 2 * i, MX_X64_RAX);
    }
}

void mx_jit_return0(mx_jit_cc_t *cc) {
    uint8_t *exit = mx_jit_return_stub(cc, MX_HOP_RETURN0);
    int32_t key_at = (int32_t)(offsetof(mx_resume_t, cache) + offsetof(mx_resume_cache_t, key));
    uint64_t key0 = ((uint64_t)1 << 48) | MX_RUN_KEY_RESUMED;
    mx_jit_return_head(cc, exit);
    mx_x64_mov_ri(&cc->hot, MX_X64_RAX, key0);
    mx_x64_cmp_mr(&cc->hot, MX_X64_RDX, key_at, MX_X64_RAX);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, exit);
    mx_jit_return_tail(cc);
    mx_jit_return_enter(cc);
}

uint32_t mx_jit_hop(mx_jit_cc_t *cc, mx_hop_t hop) {
    uintptr_t *ip = cc->ip;
    if (hop == MX_HOP_RETURN0 || hop == MX_HOP_RETURN1 || hop == MX_HOP_RETURN) {
        if (cc->ninline != 0 && hop != MX_HOP_RETURN) {
            const uintptr_t *arg = NULL;
            if (hop == MX_HOP_RETURN1) {
                arg = ip + 1;
            }
            return mx_jit_return_inline(cc, arg);
        }
        if (cc->ninline != 0) {
            mx_jit_flush(cc);
        }
        if (hop == MX_HOP_RETURN0) {
            mx_jit_return0(cc);
        } else if (hop == MX_HOP_RETURN1) {
            mx_jit_return1(cc);
        } else {
            mx_jit_return_n(cc);
        }
        return 0;
    } else if (hop == MX_HOP_GOTO || hop == MX_HOP_GOTO_HOT) {
        uintptr_t *code = (uintptr_t *)ip[2];
        if (mx_jit_can_stitch(cc, code)) {
            return mx_jit_continue(cc, code);
        }
        const uint64_t *live = mx_jit_live(cc, (mx_ir_block_t *)ip[1]);
        mx_jit_flush_live(cc, &cc->hot, UINT32_MAX, live, live);
        mx_jit_edge_jmp(cc, &ip[2], 1 << 1);
        return 0;
    }
    uint32_t out = (uint32_t)ip[1];
    uint32_t a = (uint32_t)ip[2];
    uint32_t b = (uint32_t)ip[3];
    uint64_t imm = ip[3];
    mx_jit_opnd_t ra = { .value = ip[2], .is_imm = false };
    mx_jit_opnd_t rb = { .value = ip[3], .is_imm = false };
    mx_jit_opnd_t ri = { .value = ip[3], .is_imm = true };
    switch (hop) {
        case MX_HOP_MOVE_R: {
            mx_jit_move_r(cc, out, a);
            return 3;
        }
        case MX_HOP_MOVE_I:
        case MX_HOP_SET_KIND: {
            mx_jit_move_i(cc, out, ip[2]);
            return 3;
        }
        case MX_HOP_ADD_I64_RR: {
            mx_jit_alu(cc, MX_X64_ALU_ADD, out, a, b);
            return 4;
        }
        case MX_HOP_ADD_I64_RI:
        case MX_HOP_ADD_I64_IR:
        case MX_HOP_ADD_I64_RI_BR_LT_RR:
        case MX_HOP_ADD_I64_RI_BR_LT_RI:
        case MX_HOP_ADD_I64_RI_BR_LE_RR:
        case MX_HOP_ADD_I64_RI_BR_LE_RI:
        case MX_HOP_ADD_I64_RI_BR_EQ_RR:
        case MX_HOP_ADD_I64_RI_BR_EQ_RI: {
            mx_jit_alu_i(cc, MX_X64_ALU_ADD, out, a, imm);
            return 4;
        }
        case MX_HOP_SUB_I64_RR: {
            mx_jit_alu(cc, MX_X64_ALU_SUB, out, a, b);
            return 4;
        }
        case MX_HOP_SUB_I64_RI: {
            mx_jit_alu_i(cc, MX_X64_ALU_SUB, out, a, imm);
            return 4;
        }
        case MX_HOP_SUB_I64_IR: {
            mx_jit_sub_ir(cc, out, a, imm);
            return 4;
        }
        case MX_HOP_MUL_I64_RR: {
            mx_jit_mul(cc, out, a, b);
            return 4;
        }
        case MX_HOP_MUL_I64_RI:
        case MX_HOP_MUL_I64_IR: {
            mx_jit_mul_i(cc, out, a, imm);
            return 4;
        }
        case MX_HOP_IDIV_I64_RR: {
            mx_jit_divmod(cc, false, out, ra, rb);
            return 5;
        }
        case MX_HOP_IDIV_I64_RI: {
            if (!mx_jit_divmod_pow2(cc, false, out, a, (int64_t)imm)) {
                mx_jit_divmod(cc, false, out, ra, ri);
            }
            return 5;
        }
        case MX_HOP_IDIV_I64_IR: {
            mx_jit_divmod(cc, false, out, ri, ra);
            return 5;
        }
        case MX_HOP_MOD_I64_RR: {
            mx_jit_divmod(cc, true, out, ra, rb);
            return 5;
        }
        case MX_HOP_MOD_I64_RI: {
            if (!mx_jit_divmod_pow2(cc, true, out, a, (int64_t)imm)) {
                mx_jit_divmod(cc, true, out, ra, ri);
            }
            return 5;
        }
        case MX_HOP_MOD_I64_IR: {
            mx_jit_divmod(cc, true, out, ri, ra);
            return 5;
        }
        case MX_HOP_BAND_I64_RR: {
            mx_jit_alu(cc, MX_X64_ALU_AND, out, a, b);
            return 4;
        }
        case MX_HOP_BAND_I64_RI:
        case MX_HOP_BAND_I64_IR: {
            mx_jit_alu_i(cc, MX_X64_ALU_AND, out, a, imm);
            return 4;
        }
        case MX_HOP_BOR_I64_RR: {
            mx_jit_alu(cc, MX_X64_ALU_OR, out, a, b);
            return 4;
        }
        case MX_HOP_BOR_I64_RI:
        case MX_HOP_BOR_I64_IR: {
            mx_jit_alu_i(cc, MX_X64_ALU_OR, out, a, imm);
            return 4;
        }
        case MX_HOP_BXOR_I64_RR: {
            mx_jit_alu(cc, MX_X64_ALU_XOR, out, a, b);
            return 4;
        }
        case MX_HOP_BXOR_I64_RI:
        case MX_HOP_BXOR_I64_IR: {
            mx_jit_alu_i(cc, MX_X64_ALU_XOR, out, a, imm);
            return 4;
        }
        case MX_HOP_SHL_I64_RI: {
            mx_jit_shift_i(cc, true, out, a, (int64_t)imm);
            return 4;
        }
        case MX_HOP_SHR_I64_RI: {
            mx_jit_shift_i(cc, false, out, a, (int64_t)imm);
            return 4;
        }
        case MX_HOP_SHL_I64_RR:
        case MX_HOP_SHL_I64_IR:
        case MX_HOP_SHR_I64_RR:
        case MX_HOP_SHR_I64_IR:
        case MX_HOP_IDIV_F64_RR:
        case MX_HOP_IDIV_F64_RI:
        case MX_HOP_IDIV_F64_IR:
        case MX_HOP_MOD_F64_RR:
        case MX_HOP_MOD_F64_RI:
        case MX_HOP_MOD_F64_IR:
        case MX_HOP_POW_F64_RR:
        case MX_HOP_POW_F64_RI:
        case MX_HOP_POW_F64_IR:
        case MX_HOP_POW_INT_INT:
        case MX_HOP_CONCAT:
        case MX_HOP_CONCAT_SI:
        case MX_HOP_CONCAT_IS: {
            mx_jit_hop_call(cc, hop);
            mx_jit_drop(cc, out);
            return 4;
        }
        case MX_HOP_ADD_F64_RR: {
            mx_jit_sse(cc, MX_X64_SSE_ADD, out, a, b);
            return 4;
        }
        case MX_HOP_ADD_F64_RI:
        case MX_HOP_ADD_F64_IR: {
            mx_jit_sse_ri(cc, MX_X64_SSE_ADD, out, a, imm);
            return 4;
        }
        case MX_HOP_SUB_F64_RR: {
            mx_jit_sse(cc, MX_X64_SSE_SUB, out, a, b);
            return 4;
        }
        case MX_HOP_SUB_F64_RI: {
            mx_jit_sse_ri(cc, MX_X64_SSE_SUB, out, a, imm);
            return 4;
        }
        case MX_HOP_SUB_F64_IR: {
            mx_jit_sse_ir(cc, MX_X64_SSE_SUB, out, a, imm);
            return 4;
        }
        case MX_HOP_MUL_F64_RR: {
            mx_jit_sse(cc, MX_X64_SSE_MUL, out, a, b);
            return 4;
        }
        case MX_HOP_MUL_F64_RI:
        case MX_HOP_MUL_F64_IR: {
            mx_jit_sse_ri(cc, MX_X64_SSE_MUL, out, a, imm);
            return 4;
        }
        case MX_HOP_DIV_F64_RR: {
            mx_jit_sse(cc, MX_X64_SSE_DIV, out, a, b);
            return 4;
        }
        case MX_HOP_DIV_F64_RI: {
            mx_jit_sse_ri(cc, MX_X64_SSE_DIV, out, a, imm);
            return 4;
        }
        case MX_HOP_DIV_F64_IR: {
            mx_jit_sse_ir(cc, MX_X64_SSE_DIV, out, a, imm);
            return 4;
        }
        case MX_HOP_TO_F64: {
            mx_jit_to_f64(cc, MX_JIT_XMM_SCRATCH, a);
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_movaps(&cc->hot, d, MX_JIT_XMM_SCRATCH);
            return 3;
        }
        case MX_HOP_DIV_INT_INT: {
            mx_jit_to_f64(cc, MX_JIT_XMM_SCRATCH, b);
            mx_jit_to_f64(cc, MX_JIT_XMM_SCRATCH - 1, a);
            mx_x64_sse_rr(&cc->hot, MX_X64_SSE_DIV, MX_JIT_XMM_SCRATCH - 1, MX_JIT_XMM_SCRATCH);
            uint32_t d = mx_jit_def_x(cc, out);
            mx_x64_movaps(&cc->hot, d, MX_JIT_XMM_SCRATCH - 1);
            return 4;
        }
        case MX_HOP_NEG_I64: {
            mx_jit_unary_i(cc, true, out, a);
            return 3;
        }
        case MX_HOP_NEG_F64: {
            mx_jit_neg_f64(cc, out, a);
            return 3;
        }
        case MX_HOP_BNOT_I64: {
            mx_jit_unary_i(cc, false, out, a);
            return 3;
        }
        case MX_HOP_LEN_STR: {
            mx_x64_reg_t s = mx_jit_use_g(cc, a);
            mx_x64_reg_t d = mx_jit_def_g(cc, out);
            mx_x64_load32(&cc->hot, d, s, (int32_t)offsetof(mx_str_t, len));
            return 3;
        }
        case MX_HOP_LEN_TABLE: {
            mx_jit_len_table(cc);
            return 4;
        }
        case MX_HOP_TOSTR_I64:
        case MX_HOP_TOSTR_F64:
        case MX_HOP_TABLE_NEW:
        case MX_HOP_VARARGS: {
            mx_jit_hop_call(cc, hop);
            mx_jit_drop(cc, out);
            return 3;
        }
        case MX_HOP_TABLE_SET_SK: {
            mx_jit_set_sk(cc);
            return 11;
        }
        case MX_HOP_TABLE_SET_IR:
        case MX_HOP_TABLE_SET_IK: {
            mx_jit_set_int(cc, hop);
            return 11;
        }
        case MX_HOP_TABLE_SET: {
            mx_jit_set_var(cc);
            return 13;
        }
        case MX_HOP_TABLE_INIT: {
            mx_jit_hop_call(cc, hop);
            return 6;
        }
        case MX_HOP_TABLE_INIT_IK: {
            mx_jit_init_ik(cc);
            return 6;
        }
        case MX_HOP_TABLE_INIT_SK: {
            mx_jit_init_sk(cc);
            return 6;
        }
        case MX_HOP_CLOSURE: {
            mx_jit_hop_call(cc, hop);
            mx_jit_drop_all(cc);
            return 4 + ((mx_ir_block_t *)ip[2])->func->nupvals;
        }
        case MX_HOP_UPVAL_SET: {
            mx_jit_upval_set(cc);
            return 4;
        }
        case MX_HOP_CLOSE: {
            mx_jit_hop_call(cc, hop);
            return 2;
        }
        case MX_HOP_UPVAL_GUARD: {
            mx_jit_upval_guard(cc);
            return 5;
        }
        case MX_HOP_BR_BOOL: {
            return mx_jit_br_bool(cc);
        }
        case MX_HOP_BR_LT_I64_RR: {
            return mx_jit_br_i(cc, MX_X64_CC_L, MX_X64_CC_G, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_LT_I64_RI: {
            return mx_jit_br_i(cc, MX_X64_CC_L, MX_X64_CC_G, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_LT_I64_IR: {
            return mx_jit_br_i(cc, MX_X64_CC_L, MX_X64_CC_G, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_LE_I64_RR: {
            return mx_jit_br_i(cc, MX_X64_CC_LE, MX_X64_CC_GE, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_LE_I64_RI: {
            return mx_jit_br_i(cc, MX_X64_CC_LE, MX_X64_CC_GE, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_LE_I64_IR: {
            return mx_jit_br_i(cc, MX_X64_CC_LE, MX_X64_CC_GE, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_EQ_I64_RR: {
            return mx_jit_br_i(cc, MX_X64_CC_E, MX_X64_CC_E, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_EQ_I64_RI: {
            return mx_jit_br_i(cc, MX_X64_CC_E, MX_X64_CC_E, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_EQ_I64_IR: {
            return mx_jit_br_i(cc, MX_X64_CC_E, MX_X64_CC_E, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_EQ_RAW: {
            return mx_jit_br_i(cc, MX_X64_CC_E, MX_X64_CC_E, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_LT_F64_RR: {
            return mx_jit_br_f(cc, 0, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_LT_F64_RI: {
            return mx_jit_br_f(cc, 0, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_LT_F64_IR: {
            return mx_jit_br_f(cc, 0, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_LE_F64_RR: {
            return mx_jit_br_f(cc, 1, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_LE_F64_RI: {
            return mx_jit_br_f(cc, 1, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_LE_F64_IR: {
            return mx_jit_br_f(cc, 1, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_EQ_F64_RR: {
            return mx_jit_br_f(cc, 2, MX_JIT_FORM_RR);
        }
        case MX_HOP_BR_EQ_F64_RI: {
            return mx_jit_br_f(cc, 2, MX_JIT_FORM_RI);
        }
        case MX_HOP_BR_EQ_F64_IR: {
            return mx_jit_br_f(cc, 2, MX_JIT_FORM_IR);
        }
        case MX_HOP_BR_LT_INT_F64:
        case MX_HOP_BR_LE_INT_F64:
        case MX_HOP_BR_EQ_INT_F64:
        case MX_HOP_BR_LT_F64_INT:
        case MX_HOP_BR_LE_F64_INT:
        case MX_HOP_BR_LT_STR:
        case MX_HOP_BR_LE_STR: {
            return mx_jit_br_rt(cc, hop);
        }
        case MX_HOP_BR_EQ_STR: {
            return mx_jit_br_eq_str(cc);
        }
        case MX_HOP_TABLE_GET_SK: {
            return mx_jit_get_sk(cc);
        }
        case MX_HOP_TABLE_GET_IR:
        case MX_HOP_TABLE_GET_IK: {
            return mx_jit_get_int(cc, hop);
        }
        case MX_HOP_METHOD_GET_SK: {
            return mx_jit_method_get(cc);
        }
        case MX_HOP_TABLE_GET: {
            return mx_jit_get_var(cc);
        }
        case MX_HOP_UPVAL_GET: {
            return mx_jit_upval_get(cc);
        }
        case MX_HOP_CALL_LUA: {
            return mx_jit_call_lua(cc);
        }
        case MX_HOP_TAILCALL_LUA: {
            mx_jit_tailcall_lua(cc);
            return 0;
        }
        case MX_HOP_CALL_FAST: {
            return mx_jit_call_fast(cc);
        }
        case MX_HOP_META_CMP: {
            mx_jit_code_call(cc, MX_JIT_SLOT_META_CMP, hop);
            return 0;
        }
        case MX_HOP_CELL: {
            mx_jit_code_call(cc, MX_JIT_SLOT_CELL, hop);
            return 0;
        }
        case MX_HOP_UNPACK: {
            mx_jit_flush(cc);
            mx_jit_call_rt(cc, &cc->hot, MX_JIT_SLOT_UNPACK, hop);
            mx_jit_drop_caller_saved(cc);
            mx_x64_test8_al(&cc->hot);
            mx_x64_jcc(&cc->hot, MX_X64_CC_E, mx_jit_exit_stub(cc));
            return 4;
        }
        case MX_HOP_CALL: {
            mx_kind_t kind = (mx_kind_t)(ip[2] >> 1);
            if (kind == MX_KIND_FFI || kind == cc->vm->kinds.cfn) {
                return mx_jit_call_ffi(cc);
            }
            mx_jit_stop(cc);
            return 0;
        }
        default: {
            mx_jit_stop(cc);
            return 0;
        }
    }
}

void mx_jit_unit(mx_jit_cc_t *cc) {
    mx_t *vm = cc->vm;
    uint8_t *hot_end = vm->jit.mem + MX_JIT_COLD_OFFSET;
    uint8_t *cold_end = vm->jit.mem + MX_JIT_REGION_BYTES;
    mx_jit_tails(cc);
    mx_jit_tail_code(cc, &cc->cold);
    cc->first_tail = cc->tail;
    uint8_t *poll = cc->cold.p;
    mx_jit_exit_to(cc, &cc->cold);
    mx_x64_cmp8_mi(&cc->hot, MX_X64_R12, (int32_t)offsetof(mx_t, interrupt), 0);
    mx_x64_jcc(&cc->hot, MX_X64_CC_NE, poll);
    for (uint32_t h = 0; h < cc->nhead; h++) {
        const mx_jit_home_t *home = &cc->head[h];
        if (home->phys >= MX_JIT_XMM_ID) {
            uint32_t x = (uint32_t)home->phys - MX_JIT_XMM_ID;
            mx_x64_movsd_load(&cc->hot, x, MX_X64_RBX, mx_jit_disp(home->reg));
            mx_jit_set(cc, &cc->x[x], home->reg, home->is_dirty);
        } else {
            mx_x64_load(&cc->hot, home->phys, MX_X64_RBX, mx_jit_disp(home->reg));
            mx_jit_set(cc, &cc->g[home->phys], home->reg, home->is_dirty);
        }
    }
    mx_jit_unpin(cc);
    cc->head_label = cc->hot.p;
    uint32_t n = MX_JIT_ENTRY_WORDS;
    while (n != 0 || cc->pend_at < cc->npend) {
        if (n == 0) {
            n = mx_jit_resume(cc);
            mx_jit_unpin(cc);
        } else {
            cc->ip += n;
            cc->hop_exit = NULL;
            if (cc->hot.p + MX_JIT_HOP_ROOM > hot_end || cc->cold.p + MX_JIT_HOP_ROOM > cold_end) {
                mx_jit_stop(cc);
                n = 0;
            } else {
                n = mx_jit_hop(cc, mx_jit_hop_of(vm, cc->ip[0]));
            }
            mx_jit_unpin(cc);
        }
    }
}

bool mx_jit_compile(mx_t *vm, mx_version_t *rb) {
    mx_jit_t *jit = &vm->jit;
    if (rb->is_hooked || jit->hot + MX_JIT_MARGIN > jit->mem + MX_JIT_COLD_OFFSET || jit->cold + MX_JIT_MARGIN > jit->mem + MX_JIT_REGION_BYTES) {
        return false;
    }
    if (jit->slots[MX_JIT_SLOT_LOOKUP] == NULL) {
        mx_x64_t a = { .p = jit->hot };
        mx_jit_stub_lookup(vm, &a);
        jit->hot = a.p;
    }
    vm->stats.jit_versions += 1;
    mx_jit_cc_t cc = {
        .vm = vm,
        .rb = rb,
        .code = rb->code,
        .first = rb->code,
        .ip = rb->code,
        .entry = jit->hot,
        .labels = jit->labels,
        .pend = jit->pend,
        .hot = (mx_x64_t) { .p = jit->hot },
        .cold = (mx_x64_t) { .p = jit->cold },
        .nregs = rb->block->func->nregs,
        .frame = rb->frame,
    };
    mx_jit_unit(&cc);
    if (cc.nsnap != 0) {
        mx_jit_cc_t again = {
            .vm = vm,
            .rb = rb,
            .code = rb->code,
            .first = rb->code,
            .ip = rb->code,
            .entry = jit->hot,
            .labels = jit->labels,
            .pend = jit->pend,
            .hot = (mx_x64_t) { .p = jit->hot },
            .cold = (mx_x64_t) { .p = jit->cold },
            .nregs = rb->block->func->nregs,
            .frame = rb->frame,
            .nhead = cc.nsnap,
        };
        memcpy(again.head, cc.snap, sizeof(mx_jit_home_t) * cc.nsnap);
        mx_jit_unit(&again);
        cc = again;
    }
    jit->hot = cc.hot.p;
    jit->cold = cc.cold.p;
    rb->is_loop = cc.nhead != 0;
    rb->code[1] = (uintptr_t)cc.entry;
    return true;
}

void mx_jit_recompile(mx_t *vm, mx_version_t *rb) {
    uint8_t *old = (uint8_t *)rb->code[1];
    rb->recompiles += 1;
    if (mx_jit_compile(vm, rb)) {
        mx_x64_t a = { .p = old };
        mx_x64_jmp(&a, (const uint8_t *)rb->code[1]);
    }
}
