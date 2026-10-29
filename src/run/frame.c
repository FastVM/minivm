
#include "frame.h"
#include "../value/obj.h"

mx_frame_iter_t mx_frame_iter(mx_thread_t *th) {
    return (mx_frame_iter_t) {
        .th = th,
        .p = th->frame,
        .rec_at = MX_THREAD_NONE,
    };
}

mx_frame_t mx_frame_next(mx_frame_iter_t *it) {
    while (it->p != MX_THREAD_NONE && it->th->slots != NULL) {
        mx_thread_t *th = it->th;
        uint32_t p = it->p;
        uintptr_t link = th->slots[p].bits;
        mx_link_tag_t tag = mx_link_tag(link);
        uint32_t rec_at = it->rec_at;
        if (tag == MX_LINK_SAFEPOINT) {
            it->rec_at = MX_THREAD_NONE;
        } else {
            it->rec_at = p + 1;
        }
        it->p = mx_link_caller(link);
        if (tag != MX_LINK_HOOK && tag != MX_LINK_META && tag != MX_LINK_SAFEPOINT) {
            const mx_resume_t *at = NULL;
            if (rec_at != MX_THREAD_NONE) {
                at = (const mx_resume_t *)th->slots[rec_at].bits;
            }
            return (mx_frame_t) {
                .th = th,
                .at = at,
                .p = p,
                .rec_at = rec_at,
                .tag = tag,
                .is_lua = mx_link_is_lua(tag),
                .is_valid = true,
            };
        }
    }
    return (mx_frame_t) { .is_valid = false };
}

mx_frame_t mx_frame_at_level(mx_thread_t *th, uint32_t level) {
    mx_frame_iter_t it = mx_frame_iter(th);
    mx_frame_t f = mx_frame_next(&it);
    for (uint32_t i = 0; i < level && f.is_valid; i++) {
        f = mx_frame_next(&it);
    }
    return f;
}

uint32_t mx_frame_count(mx_thread_t *th) {
    mx_frame_iter_t it = mx_frame_iter(th);
    uint32_t n = 0;
    for (mx_frame_t f = mx_frame_next(&it); f.is_valid; f = mx_frame_next(&it)) {
        n += 1;
    }
    return n;
}

mx_val_t *mx_frame_base(const mx_frame_t *f) {
    return f->th->slots + f->p + 2;
}

mx_obj_t mx_frame_fn(const mx_frame_t *f) {
    if (f->is_lua) {
        return mx_obj_of_raw(MX_KIND_CLOSURE, mx_frame_base(f)[0]);
    } else {
        return *mx_thread_objs(f->th, f->p + 2);
    }
}

mx_ir_block_t *mx_frame_func(const mx_frame_t *f) {
    return mx_obj_get_closure(mx_frame_fn(f))->block;
}

int64_t mx_frame_line(const mx_frame_t *f) {
    if (!f->is_lua || f->at == NULL) {
        return -1;
    }
    mx_location_range_t range = mx_resume_range(f->at);
    if (range.source == NULL) {
        return -1;
    } else {
        return (int64_t)mx_source_line(range.source, range.start);
    }
}

mx_obj_t mx_frame_reg(const mx_frame_t *f, uint32_t reg) {
    const mx_ir_func_t *func = mx_frame_func(f)->func;
    if (f->at == NULL || reg >= func->nregs) {
        return mx_obj_of_nil();
    }
    mx_val_t *base = mx_frame_base(f);
    mx_kind_t kind = MX_KIND_NIL;
    if (reg < f->at->nkinds) {
        kind = f->at->kinds[reg];
    }
    if (kind == MX_KIND_CELL_ANY || kind == MX_KIND_SLOT) {
        kind = (mx_kind_t)base[func->nregs + reg].bits;
    } else {
        kind = mx_kind_plain(kind);
    }
    return mx_obj_of_raw(kind, base[reg]);
}

bool mx_frame_set_reg(mx_t *vm, const mx_frame_t *f, uint32_t reg, mx_obj_t v) {
    const mx_ir_func_t *func = mx_frame_func(f)->func;
    if (f->at == NULL || f->rec_at == MX_THREAD_NONE || reg >= func->nregs) {
        return false;
    }
    mx_val_t *base = mx_frame_base(f);
    mx_kind_t old = MX_KIND_NIL;
    if (reg < f->at->nkinds) {
        old = f->at->kinds[reg];
    }
    base[reg] = v.val;
    if (mx_kind_is_cell(old)) {
        base[func->nregs + reg].bits = v.kind;
    }
    if (old == MX_KIND_CELL_ANY || old == MX_KIND_SLOT || mx_kind_plain(old) == v.kind) {
        return true;
    }
    uint32_t n = f->at->nkinds;
    if (reg + 1 > n) {
        n = reg + 1;
    }
    mx_kind_t *kinds = mx_mem_calloc(sizeof(mx_kind_t) * n);
    memcpy(kinds, f->at->kinds, sizeof(mx_kind_t) * f->at->nkinds);
    if (mx_kind_is_open(old)) {
        kinds[reg] = MX_KIND_CELL | v.kind;
    } else {
        kinds[reg] = v.kind;
    }
    mx_resume_t *over = mx_ir_alloc(vm, sizeof(mx_resume_t));
    *over = *f->at;
    over->kinds = mx_version_kinds_intern(vm, kinds, n);
    over->nkinds = n;
    over->is_override = true;
    over->cache = (mx_resume_cache_t) {0};
    mx_mem_free(kinds);
    f->th->slots[f->rec_at].bits = (uintptr_t)over;
    return true;
}

const mx_ir_local_t *mx_frame_local(const mx_frame_t *f, int64_t n) {
    if (!f->is_lua || f->at == NULL || n < 1) {
        return NULL;
    }
    const mx_ir_block_t *head = mx_frame_func(f);
    const mx_ir_func_t *func = head->func;
    uint32_t pos = mx_resume_range(f->at).start;
    int64_t seen = 0;
    for (uint32_t i = 0; i < func->locals.len; i++) {
        const mx_ir_local_t *local = &func->locals.ptr[i];
        if (!(local->is_pack && head->is_main) && local->start <= pos && pos < local->stop) {
            seen += 1;
            if (seen == n) {
                return local;
            }
        }
    }
    return NULL;
}
