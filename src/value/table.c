
#include "table.h"
#include "obj.h"
#include "string.h"
#include "../vm/gc.h"

mx_obj_table_t *mx_table_new(mx_t *vm) {
    return mx_table_init(vm, sizeof(mx_obj_table_t), false);
}

mx_obj_table_t *mx_table_new_short(mx_t *vm) {
    return mx_table_init(vm, MX_TABLE_SHORT_SIZE, true);
}

mx_obj_table_t *mx_table_new_sized(mx_t *vm, uint32_t narray, uint32_t nhash) {
    mx_obj_table_t *tbl;
    if (narray >= 1 && narray <= MX_TABLE_INLINE_ARRAY) {
        tbl = mx_table_init(vm, sizeof(mx_obj_table_t), false);
        tbl->array = tbl->inline_array;
        tbl->array_cap = MX_TABLE_INLINE_ARRAY;
    } else {
        tbl = mx_table_init(vm, MX_TABLE_SHORT_SIZE, true);
        if (narray != 0) {
            mx_table_array_reserve(vm, tbl, narray);
        }
    }
    if (nhash != 0) {
        mx_table_hash_reserve(vm, tbl, nhash);
    }
    return tbl;
}

mx_obj_table_t *mx_table_new_pack(mx_t *vm, const mx_obj_t *src, uint32_t count, bool has_n) {
    mx_obj_table_t *pack;
    if (count >= 1 && count <= MX_TABLE_INLINE_ARRAY) {
        pack = mx_table_new(vm);
    } else {
        pack = mx_table_new_short(vm);
    }
    if (count > 0) {
        mx_obj_t *arr;
        if (count <= MX_TABLE_INLINE_ARRAY) {
            arr = pack->inline_array;
            pack->array_cap = MX_TABLE_INLINE_ARRAY;
        } else {
            arr = mx_gc_pool_alloc(vm, sizeof(mx_obj_t) * count);
            mx_gc_note_bytes(vm, sizeof(mx_obj_t) * count);
            mx_gc_note_final_owned(&pack->head);
            pack->array_cap = count;
        }
        for (uint32_t i = 0; i < count; i++) {
            arr[i] = src[i];
        }
        pack->array = arr;
        pack->array_len = count;
        uint32_t border = count;
        while (border > 0 && mx_obj_is_nil(arr[border - 1])) {
            border -= 1;
        }
        pack->len = border;
    }
    if (!has_n) {
        return pack;
    }
    mx_obj_t nkey = vm->char_strs[(uint8_t)'n'];
    if (nkey.kind == MX_KIND_NIL) {
        nkey = mx_obj_intern_strn(vm, "n", 1);
        vm->char_strs[(uint8_t)'n'] = nkey;
    }
    mx_table_hash_set(vm, pack, nkey, mx_obj_of_int((int64_t)count));
    return pack;
}

uint32_t mx_table_key_hash(mx_obj_t key) {
    if (mx_obj_is_int(key)) {
        int64_t n = mx_obj_get_int(key);
        return (uint32_t)((uint64_t)n ^ ((uint64_t)n >> MX_TABLE_HASH_FOLD)) * MX_TABLE_HASH_INT_MULT;
    } else if (mx_obj_is_float(key)) {
        double n = mx_obj_get_float(key);
        if (mx_obj_float_fits_int(n) && floor(n) == n) {
            int64_t i = (int64_t)n;
            return (uint32_t)((uint64_t)i ^ ((uint64_t)i >> MX_TABLE_HASH_FOLD)) * MX_TABLE_HASH_INT_MULT;
        } else {
            uint64_t bits = key.val.bits;
            return (uint32_t)(bits ^ (bits >> MX_TABLE_HASH_FOLD)) * MX_TABLE_HASH_FLOAT_MULT;
        }
    } else if (mx_obj_is_string(key)) {
        return mx_str_content_hash(mx_obj_get_string(key));
    } else if (mx_obj_is_boolean(key)) {
        return UINT32_MAX - (uint32_t)mx_obj_get_boolean(key);
    } else {
        return (uint32_t)(size_t)mx_ref_obj(key.val.ref) >> MX_TABLE_HASH_PTR_SHIFT;
    }
}

bool mx_table_key_eq(mx_obj_t a, mx_obj_t b) {
    if (mx_obj_is_number(a) && mx_obj_is_number(b)) {
        return mx_obj_num_eq(a, b);
    } else if (a.kind != b.kind) {
        return false;
    } else if (mx_obj_is_string(a)) {
        mx_str_t *b1 = mx_obj_get_string(a);
        mx_str_t *b2 = mx_obj_get_string(b);
        if (b1 == b2) {
            return true;
        } else if (b1->hash != 0 && b2->hash != 0 && b1->hash != b2->hash) {
            return false;
        } else {
            return b1->len == b2->len && memcmp(b1->buf, b2->buf, b1->len) == 0;
        }
    } else {
        return a.val.bits == b.val.bits;
    }
}

mx_obj_t mx_table_get(mx_obj_table_t *tbl, mx_obj_t key) {
    return mx_table_get_inline(tbl, key);
}

mx_obj_t mx_table_get_slow(mx_obj_table_t *tbl, mx_obj_t key) {
    if (mx_obj_is_int(key)) {
        int64_t i = mx_obj_get_int(key);
        if (1 <= i && i <= (int64_t)tbl->array_len) {
            return tbl->array[i - 1];
        }
    } else if (mx_obj_is_float(key)) {
        double f = mx_obj_get_float(key);
        if (1 <= f && f <= (double)tbl->array_len) {
            uint32_t i = (uint32_t)f;
            if ((double)i == f) {
                return tbl->array[i - 1];
            }
        }
    }
    if (tbl->used == 0) {
        return mx_obj_of_nil();
    }
    size_t len = mx_table_hash_cap(tbl);
    size_t mask = len - 1;
    if (mx_obj_is_string(key)) {
        mx_str_t *kb = mx_obj_get_string(key);
        uint32_t hash;
        if (kb->hash == 0) {
            hash = mx_table_key_hash(key);
        } else {
            hash = kb->hash;
        }
        size_t look = hash & mask;
        for (size_t probe = 0; probe < len; probe++) {
            const mx_table_entry_t *e = &tbl->entries[look];
            if (e->key_kind == MX_KIND_NIL) {
                return mx_obj_of_nil();
            } else if (e->key_kind == MX_KIND_STRING && e->hash == hash) {
                if (e->key.bits == key.val.bits) {
                    return mx_table_entry_val(e);
                }
                mx_str_t *fb = mx_obj_get_string(mx_table_entry_key(e));
                if (fb->len == kb->len && memcmp(fb->buf, kb->buf, kb->len) == 0) {
                    return mx_table_entry_val(e);
                }
            }
            look = (look + 1) & mask;
        }
        return mx_obj_of_nil();
    } else {
        size_t look = mx_table_key_hash(key) & mask;
        for (size_t probe = 0; probe < len; probe++) {
            const mx_table_entry_t *e = &tbl->entries[look];
            mx_obj_t found_key = mx_table_entry_key(e);
            if (mx_obj_is_nil(found_key)) {
                return mx_obj_of_nil();
            } else if (found_key.kind == key.kind && found_key.val.bits == key.val.bits) {
                return mx_table_entry_val(e);
            } else if ((found_key.kind == key.kind || (mx_obj_is_number(key) && mx_obj_is_number(found_key)))
                && mx_table_key_eq(key, found_key)) {
                return mx_table_entry_val(e);
            }
            look = (look + 1) & mask;
        }
        return mx_obj_of_nil();
    }
}

mx_table_slot_t mx_table_get_string_slot(mx_obj_table_t *tbl, mx_obj_t key) {
    if (tbl->used == 0 || tbl->entries == NULL) {
        return (mx_table_slot_t) { .value = mx_obj_of_nil(), .slot = -1 };
    }
    size_t len = mx_table_hash_cap(tbl);
    size_t mask = len - 1;
    mx_str_t *kb = mx_obj_get_string(key);
    uint32_t hash;
    if (kb->hash == 0) {
        hash = mx_table_key_hash(key);
    } else {
        hash = kb->hash;
    }
    size_t look = hash & mask;
    for (size_t probe = 0; probe < len; probe++) {
        const mx_table_entry_t *e = &tbl->entries[look];
        if (e->key_kind == MX_KIND_NIL) {
            return (mx_table_slot_t) { .value = mx_obj_of_nil(), .slot = -1 };
        } else if (e->key_kind == MX_KIND_STRING && e->hash == hash) {
            if (e->key.bits == key.val.bits) {
                return (mx_table_slot_t) { .value = mx_table_entry_val(e), .slot = (int32_t)look };
            } else {
                mx_str_t *fb = mx_obj_get_string(mx_table_entry_key(e));
                if (fb->len == kb->len && memcmp(fb->buf, kb->buf, kb->len) == 0) {
                    return (mx_table_slot_t) { .value = mx_table_entry_val(e), .slot = (int32_t)look };
                }
            }
        }
        look = (look + 1) & mask;
    }
    return (mx_table_slot_t) { .value = mx_obj_of_nil(), .slot = -1 };
}

int32_t mx_table_hash_slot(mx_obj_table_t *tbl, mx_obj_t key) {
    if (tbl->used == 0 || tbl->entries == NULL) {
        return -1;
    }
    size_t len = mx_table_hash_cap(tbl);
    size_t mask = len - 1;
    size_t look = mx_table_key_hash(key) & mask;
    for (size_t probe = 0; probe < len; probe++) {
        mx_obj_t found_key = mx_table_entry_key(&tbl->entries[look]);
        if (mx_obj_is_nil(found_key)) {
            return -1;
        } else if (mx_table_key_eq(key, found_key)) {
            return (int32_t)look;
        }
        look = (look + 1) & mask;
    }
    return -1;
}

mx_table_entry_t *mx_table_entries_new(mx_t *vm, uint8_t size) {
    size_t count = mx_table_hash_cap_of(size);
    mx_table_entry_t *entries = mx_gc_pool_alloc(vm, sizeof(mx_table_entry_t) * count);
    memset(entries, 0, sizeof(mx_table_entry_t) * count);
    mx_gc_note_bytes(vm, sizeof(mx_table_entry_t) * count);
    return entries;
}

void mx_table_entries_free(mx_t *vm, mx_table_entry_t *entries, uint8_t size) {
    mx_gc_pool_free(vm, entries, sizeof(mx_table_entry_t) * mx_table_hash_cap_of(size));
}

void mx_table_hash_reserve(mx_t *vm, mx_obj_table_t *restrict tbl, uint32_t count) {
    if (tbl->entries == NULL) {
        uint8_t nsize = 0;
        bool fits = false;
        while (!fits) {
            size_t cap = mx_table_hash_cap_of(nsize);
            size_t allowed;
            if (cap <= MX_TABLE_HASH_MIN_CAP) {
                allowed = cap;
            } else {
                allowed = cap * MX_TABLE_LOAD_NUM / MX_TABLE_LOAD_DEN;
            }
            fits = allowed >= (size_t)count;
            if (!fits) {
                nsize += 1;
            }
        }
        tbl->size = nsize;
        tbl->entries = mx_table_entries_new(vm, nsize);
        mx_gc_note_final_owned(&tbl->head);
    }
}

void mx_table_hash_grow(mx_t *vm, mx_obj_table_t *restrict tbl) {
    size_t olen = mx_table_hash_cap(tbl);
    size_t live = 0;
    for (size_t i = 0; i < olen; i++) {
        if (tbl->entries[i].key_kind != MX_KIND_NIL && tbl->entries[i].val_kind != MX_KIND_NIL) {
            live += 1;
        }
    }
    uint8_t nsize = 0;
    while (mx_table_hash_cap_of(nsize) < live * 2) {
        nsize += 1;
    }
    size_t nlen = mx_table_hash_cap_of(nsize);
    mx_table_entry_t *nentries = mx_table_entries_new(vm, nsize);
    uint32_t used = 0;
    for (size_t i = 0; i < olen; i++) {
        const mx_table_entry_t *e = &tbl->entries[i];
        if (e->key_kind == MX_KIND_NIL || e->val_kind == MX_KIND_NIL) {
            continue;
        }
        size_t look = e->hash & (nlen - 1);
        while (nentries[look].key_kind != MX_KIND_NIL) {
            look = (look + 1) & (nlen - 1);
        }
        nentries[look] = *e;
        used += 1;
    }
    mx_table_entries_free(vm, tbl->entries, tbl->size);
    tbl->entries = nentries;
    tbl->size = nsize;
    tbl->used = used;
}

void mx_table_extend_len(mx_obj_table_t *restrict tbl, uint32_t from) {
    uint32_t next = from;
    while (next < tbl->array_len && !mx_obj_is_nil(tbl->array[next])) {
        next += 1;
    }
    if (next == tbl->array_len) {
        while (!mx_obj_is_nil(mx_table_get(tbl, mx_obj_of_int((int64_t)next + 1)))) {
            next += 1;
        }
    }
    tbl->len = next;
}

int32_t mx_table_hash_set(mx_t *vm, mx_obj_table_t *restrict tbl, mx_obj_t raw, mx_obj_t value) {
    mx_obj_int_t as_int = mx_obj_to_int(raw);
    mx_obj_t key;
    if (mx_obj_is_float(raw) && as_int.is_int) {
        key = mx_obj_of_int(as_int.value);
    } else {
        key = raw;
    }
    if (tbl->entries == NULL) {
        if (mx_obj_is_nil(value)) {
            return -1;
        }
        tbl->entries = mx_table_entries_new(vm, tbl->size);
        mx_gc_note_final_owned(&tbl->head);
    }
    mx_gc_write_barrier(vm, tbl, value);
    mx_gc_write_barrier(vm, tbl, key);
    size_t len = mx_table_hash_cap(tbl);
    uint32_t hash;
    if (key.kind == MX_KIND_STRING && !((mx_str_t *)mx_ref_obj(key.val.ref))->is_rope && ((mx_str_t *)mx_ref_obj(key.val.ref))->hash != 0) {
        hash = ((mx_str_t *)mx_ref_obj(key.val.ref))->hash;
    } else {
        hash = mx_table_key_hash(key);
    }
    size_t first_look = hash & (len - 1);
    size_t look = first_look;
    for (size_t i = 0; i < len; i++) {
        mx_table_entry_t *e = &tbl->entries[look];
        mx_obj_t table_key = mx_table_entry_key(e);
        if (mx_obj_is_nil(table_key)) {
            break;
        }
        bool is_same = (table_key.kind == key.kind && table_key.val.bits == key.val.bits)
            || ((table_key.kind == key.kind || (mx_obj_is_number(key) && mx_obj_is_number(table_key))) && mx_table_key_eq(key, table_key));
        if (is_same) {
            bool was_dead = e->val_kind == MX_KIND_NIL;
            if (mx_obj_is_nil(value)) {
                if (mx_obj_is_number(key)) {
                    double f = mx_obj_get_number(key);
                    if (1 <= f && f <= tbl->len) {
                        uint32_t at = (uint32_t)f;
                        if ((double)at == f) {
                            tbl->len = at - 1;
                        }
                    }
                }
            }
            mx_table_entry_set_val(e, value);
            if (mx_obj_is_nil(value) && !was_dead) {
                size_t next_look = (look + 1) & (len - 1);
                if (look == first_look && tbl->entries[next_look].key_kind == MX_KIND_NIL) {
                    mx_table_entry_set_key(e, mx_obj_of_nil(), 0);
                    tbl->used -= 1;
                }
            }
            if (was_dead && !mx_obj_is_nil(value)) {
                int32_t next = tbl->len + 1;
                if (mx_obj_is_number(key) && mx_obj_get_number(key) == next) {
                    mx_table_extend_len(tbl, (uint32_t)next);
                }
            }
            return (int32_t)look;
        }
        look = (look + 1) & (len - 1);
    }
    size_t slot;
    if (mx_obj_is_nil(value)) {
        return -1;
    } else if (((tbl->used + 1) * MX_TABLE_LOAD_DEN > len * MX_TABLE_LOAD_NUM
            && len > MX_TABLE_HASH_MIN_CAP)
        || tbl->entries[look].key_kind != MX_KIND_NIL) {
        mx_table_hash_grow(vm, tbl);
        size_t cap = mx_table_hash_cap(tbl);
        size_t at = hash & (cap - 1);
        while (tbl->entries[at].key_kind != MX_KIND_NIL) {
            at = (at + 1) & (cap - 1);
        }
        slot = at;
    } else {
        slot = look;
    }
    if (key.kind == MX_KIND_CLOSURE || key.kind >= MX_KIND_FIRST_DATA) {
        mx_gc_note_ident_key(vm, mx_ref_obj(key.val.ref));
    }
    if (mx_obj_is_int(key)) {
        tbl->has_int_hash = true;
    }
    tbl->used += 1;
    mx_table_entry_set_key(&tbl->entries[slot], key, hash);
    mx_table_entry_set_val(&tbl->entries[slot], value);
    int32_t next = tbl->len + 1;
    if (mx_obj_is_number(key) && mx_obj_get_number(key) == next) {
        mx_table_extend_len(tbl, (uint32_t)next);
    }
    return (int32_t)slot;
}

mx_table_steal_t mx_table_hash_steal(mx_obj_table_t *restrict tbl, uint32_t key) {
    if (tbl->used == 0 || !tbl->has_int_hash) {
        return (mx_table_steal_t) { .value = mx_obj_of_nil(), .is_stolen = false };
    }
    mx_obj_t boxed = mx_obj_of_int((int64_t)key);
    size_t len = mx_table_hash_cap(tbl);
    size_t look = mx_table_key_hash(boxed) & (len - 1);
    for (size_t i = 0; i < len; i++) {
        mx_table_entry_t *e = &tbl->entries[look];
        mx_obj_t table_key = mx_table_entry_key(e);
        if (mx_obj_is_nil(table_key)) {
            break;
        } else if (mx_table_key_eq(boxed, table_key)) {
            if (e->val_kind == MX_KIND_NIL) {
                break;
            } else {
                mx_obj_t got = mx_table_entry_val(e);
                mx_table_entry_set_val(e, mx_obj_of_nil());
                return (mx_table_steal_t) { .value = got, .is_stolen = true };
            }
        }
        look = (look + 1) & (len - 1);
    }
    return (mx_table_steal_t) { .value = mx_obj_of_nil(), .is_stolen = false };
}

void mx_table_array_push(mx_t *vm, mx_obj_table_t *restrict tbl, mx_obj_t value) {
    mx_gc_write_barrier(vm, tbl, value);
    if (tbl->array_len == tbl->array_cap) {
        if (tbl->array_cap == 0 && tbl->is_short) {
            tbl->array = mx_gc_pool_alloc(vm, sizeof(mx_obj_t) * MX_TABLE_INLINE_ARRAY);
            mx_gc_note_bytes(vm, sizeof(mx_obj_t) * MX_TABLE_INLINE_ARRAY);
            mx_gc_note_final_owned(&tbl->head);
            tbl->array_cap = MX_TABLE_INLINE_ARRAY;
        } else if (tbl->array_cap == 0) {
            tbl->array = tbl->inline_array;
            tbl->array_cap = MX_TABLE_INLINE_ARRAY;
        } else {
            uint32_t ncap = tbl->array_cap * 2;
            mx_gc_note_bytes(vm, sizeof(mx_obj_t) * ncap);
            if (tbl->array != tbl->inline_array) {
                tbl->array = mx_gc_pool_realloc(vm, tbl->array, sizeof(mx_obj_t) * tbl->array_cap, sizeof(mx_obj_t) * ncap);
            } else {
                mx_obj_t *narray = mx_gc_pool_alloc(vm, sizeof(mx_obj_t) * ncap);
                memcpy(narray, tbl->array, sizeof(mx_obj_t) * tbl->array_len);
                mx_gc_note_final_owned(&tbl->head);
                tbl->array = narray;
            }
            tbl->array_cap = ncap;
        }
    }
    tbl->array[tbl->array_len] = value;
    tbl->array_len += 1;
}

void mx_table_array_free(mx_t *vm, mx_obj_t *array, uint32_t cap) {
    mx_gc_pool_free(vm, array, sizeof(mx_obj_t) * cap);
}

void mx_table_array_reserve(mx_t *vm, mx_obj_table_t *restrict tbl, uint32_t cap) {
    bool have = cap <= tbl->array_cap;
    if (!have && cap <= MX_TABLE_INLINE_ARRAY && tbl->array_cap == 0 && !tbl->is_short) {
        tbl->array = tbl->inline_array;
        tbl->array_cap = MX_TABLE_INLINE_ARRAY;
    } else if (!have) {
        mx_gc_note_bytes(vm, sizeof(mx_obj_t) * cap);
        if (tbl->array != NULL && tbl->array != tbl->inline_array) {
            tbl->array = mx_gc_pool_realloc(vm, tbl->array, sizeof(mx_obj_t) * tbl->array_cap, sizeof(mx_obj_t) * cap);
        } else {
            mx_obj_t *narray = mx_gc_pool_alloc(vm, sizeof(mx_obj_t) * cap);
            if (tbl->array != NULL) {
                memcpy(narray, tbl->array, sizeof(mx_obj_t) * tbl->array_len);
            }
            mx_gc_note_final_owned(&tbl->head);
            tbl->array = narray;
        }
        tbl->array_cap = cap;
    }
}

void mx_table_set(mx_t *vm, mx_obj_table_t *restrict tbl, mx_obj_t key, mx_obj_t value) {
    mx_obj_int_t idx = mx_obj_to_int(key);
    if (idx.is_int && 1 <= idx.value && idx.value <= (int64_t)tbl->array_len) {
        mx_gc_write_barrier(vm, tbl, value);
        uint32_t i = (uint32_t)idx.value;
        bool was_nil = mx_obj_is_nil(tbl->array[i - 1]);
        tbl->array[i - 1] = value;
        if (mx_obj_is_nil(value)) {
            if (i <= tbl->len) {
                tbl->len = i - 1;
            }
        } else if (was_nil && i == tbl->len + 1) {
            mx_table_extend_len(tbl, i);
        }
    } else if (idx.is_int && idx.value == (int64_t)tbl->array_len + 1 && !mx_obj_is_nil(value)) {
        mx_table_array_push(vm, tbl, value);
        if (tbl->used != 0) {
            mx_table_steal_t stolen = mx_table_hash_steal(tbl, tbl->array_len + 1);
            while (stolen.is_stolen) {
                mx_table_array_push(vm, tbl, stolen.value);
                stolen = mx_table_hash_steal(tbl, tbl->array_len + 1);
            }
        }
        if ((uint32_t)idx.value == tbl->len + 1) {
            tbl->len = tbl->array_len;
        }
    } else {
        mx_table_hash_set(vm, tbl, key, value);
    }
}

mx_table_pair_t mx_table_pairs_hash_from(mx_t *vm, mx_obj_table_t *tbl, size_t slot) {
    if (tbl->entries != NULL) {
        size_t len = mx_table_hash_cap(tbl);
        for (size_t i = slot; i < len; i++) {
            const mx_table_entry_t *e = &tbl->entries[i];
            if (e->key_kind != MX_KIND_NIL && e->val_kind != MX_KIND_NIL) {
                if (tbl->head.kind == vm->kinds.table) {
                    tbl->hidden = (uint64_t)i + 1;
                }
                return (mx_table_pair_t) { .key = mx_table_entry_key(e), .val = mx_table_entry_val(e), .is_valid = true };
            }
        }
    }
    return (mx_table_pair_t) { .key = mx_obj_of_nil(), .val = mx_obj_of_nil(), .is_valid = true };
}

mx_table_pair_t mx_table_pairs_array_from(mx_t *vm, mx_obj_table_t *tbl, size_t astart) {
    for (size_t i = astart; i < tbl->array_len; i++) {
        if (!mx_obj_is_nil(tbl->array[i])) {
            return (mx_table_pair_t) { .key = mx_obj_of_int((int64_t)(i + 1)), .val = tbl->array[i], .is_valid = true };
        }
    }
    return mx_table_pairs_hash_from(vm, tbl, 0);
}

mx_table_step_t mx_table_next_into(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key, mx_obj_t *out) {
    bool is_array_key = mx_obj_is_nil(key) || (mx_obj_is_int(key) && key.val.i64 >= 1 && (uint64_t)key.val.i64 <= tbl->array_len);
    if (is_array_key) {
        size_t from;
        if (mx_obj_is_nil(key)) {
            from = 0;
        } else {
            from = (size_t)key.val.i64;
        }
        for (size_t i = from; i < tbl->array_len; i++) {
            if (!mx_obj_is_nil(tbl->array[i])) {
                out[0] = mx_obj_of_int((int64_t)(i + 1));
                out[1] = tbl->array[i];
                return MX_TABLE_STEP_PAIR;
            }
        }
    }
    if (tbl->entries == NULL) {
        if (is_array_key) {
            return MX_TABLE_STEP_END;
        } else {
            return MX_TABLE_STEP_INVALID;
        }
    }
    mx_table_pair_t pair;
    if (is_array_key) {
        pair = mx_table_pairs_hash_from(vm, tbl, 0);
    } else {
        pair = mx_table_pairs_next(vm, tbl, key);
    }
    if (!pair.is_valid) {
        return MX_TABLE_STEP_INVALID;
    } else if (mx_obj_is_nil(pair.key)) {
        return MX_TABLE_STEP_END;
    } else {
        out[0] = pair.key;
        out[1] = pair.val;
        return MX_TABLE_STEP_PAIR;
    }
}

mx_table_pair_t mx_table_pairs_next(mx_t *vm, mx_obj_table_t *tbl, mx_obj_t key) {
    if (mx_obj_is_nil(key)) {
        return mx_table_pairs_array_from(vm, tbl, 0);
    } else if (mx_obj_is_number(key)) {
        double d = mx_obj_get_number(key);
        int64_t i = (int64_t)d;
        if ((double)i == d && i >= 1 && (uint64_t)i <= tbl->array_len) {
            return mx_table_pairs_array_from(vm, tbl, (size_t)i);
        }
    }
    if (tbl->entries == NULL) {
        return (mx_table_pair_t) { .key = mx_obj_of_nil(), .val = mx_obj_of_nil(), .is_valid = false };
    }
    size_t len = mx_table_hash_cap(tbl);
    if (tbl->head.kind == vm->kinds.table && tbl->hidden != 0 && tbl->hidden <= len) {
        size_t hint = (size_t)tbl->hidden - 1;
        const mx_table_entry_t *he = &tbl->entries[hint];
        if ((he->key_kind == key.kind && he->key.bits == key.val.bits) || mx_table_entry_was_key(he, key)) {
            return mx_table_pairs_hash_from(vm, tbl, hint + 1);
        }
    }
    size_t init_look = mx_table_key_hash(key) & (len - 1);
    size_t look = init_look;
    for (size_t probe = 0; probe < len; probe++) {
        mx_obj_t found_key = mx_table_entry_key(&tbl->entries[look]);
        if (mx_obj_is_nil(found_key)) {
            break;
        }
        if (mx_table_key_eq(key, found_key) || mx_table_entry_was_key(&tbl->entries[look], key)) {
            return mx_table_pairs_hash_from(vm, tbl, look + 1);
        }
        look = (look + 1) & (len - 1);
    }
    return mx_table_pairs_hash_from(vm, tbl, init_look + 1);
}
