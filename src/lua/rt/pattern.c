
#include "pattern.h"

bool mx_pattern_set_has(const uint64_t *set, uint8_t c) {
    return ((set[c >> 6] >> (c & 63)) & 1) != 0;
}

void mx_pattern_set_add(uint64_t *set, uint8_t c) {
    set[c >> 6] |= (uint64_t)1 << (c & 63);
}

void mx_pattern_set_escape(mx_pattern_cache_t *cache, uint64_t *set, uint8_t cl) {
    int32_t lower = tolower(cl);
    if (!mx_pattern_is_class(lower)) {
        mx_pattern_set_add(set, cl);
        return;
    }
    uint32_t index = (uint32_t)(lower - 'a');
    uint64_t *cached = cache->sets[index];
    if ((cache->ready & ((uint32_t)1 << index)) == 0) {
        memset(cached, 0, sizeof(cache->sets[index]));
        for (int32_t c = 0; c < 256; c++) {
            if (mx_pattern_class_test(lower, c)) {
                mx_pattern_set_add(cached, (uint8_t)c);
            }
        }
        cache->ready |= (uint32_t)1 << index;
    }
    bool is_negated = isupper(cl) != 0;
    for (size_t w = 0; w < 4; w++) {
        if (is_negated) {
            set[w] |= ~cached[w];
        } else {
            set[w] |= cached[w];
        }
    }
}

mx_pattern_set_end_t mx_pattern_set_parse(mx_pattern_cache_t *cache, uint64_t *set, const char *pat, size_t plen, size_t at) {
    size_t first = at + 1;
    bool negate = first < plen && pat[first] == '^';
    if (negate) {
        first += 1;
    }
    size_t close = first;
    do {
        if (close >= plen) {
            return (mx_pattern_set_end_t) { .next = plen, .is_closed = false };
        }
        close += 1;
        if (pat[close - 1] == '%' && close < plen) {
            close += 1;
        }
    } while (close >= plen || pat[close] != ']');
    size_t i = first;
    while (i < close) {
        uint8_t c = (uint8_t)pat[i];
        if (c == '%') {
            mx_pattern_set_escape(cache, set, (uint8_t)pat[i + 1]);
            i += 2;
        } else if (i + 2 < close && pat[i + 1] == '-') {
            for (int32_t r = c; r <= (uint8_t)pat[i + 2]; r++) {
                mx_pattern_set_add(set, (uint8_t)r);
            }
            i += 3;
        } else {
            mx_pattern_set_add(set, c);
            i += 1;
        }
    }
    if (negate) {
        for (size_t w = 0; w < 4; w++) {
            set[w] = ~set[w];
        }
    }
    return (mx_pattern_set_end_t) { .next = close + 1, .is_closed = true };
}

mx_pattern_item_t *mx_pattern_emit(mx_pattern_prog_t *prog, mx_pattern_op_t op) {
    mx_pattern_item_t *item = &prog->items[prog->len];
    prog->len += 1;
    *item = (mx_pattern_item_t) { .op = op };
    return item;
}

void mx_pattern_emit_fail(mx_pattern_prog_t *prog, const char *error, uint8_t arg) {
    mx_pattern_item_t *item = mx_pattern_emit(prog, MX_PATTERN_OP_FAIL);
    item->error = error;
    item->a = arg;
}

void mx_pattern_compile(mx_pattern_cache_t *cache, mx_pattern_prog_t *prog, const char *pat, size_t plen) {
    prog->items = prog->inline_items;
    if (plen + 1 > MX_PATTERN_ITEMS_INLINE) {
        prog->items = mx_mem_alloc(sizeof(mx_pattern_item_t) * (plen + 1));
    }
    prog->len = 0;
    prog->ncap = 0;
    prog->first_byte = -1;
    uint8_t open[MX_PATTERN_MAX_CAPTURES];
    uint32_t nopen = 0;
    bool closed[MX_PATTERN_MAX_CAPTURES];
    size_t p = 0;
    bool is_failed = false;
    while (p < plen && !is_failed) {
        char c = pat[p];
        if (c == '(' && prog->ncap >= MX_PATTERN_MAX_CAPTURES) {
            mx_pattern_emit_fail(prog, "too many captures", 0);
            is_failed = true;
        } else if (c == '(' && p + 1 < plen && pat[p + 1] == ')') {
            mx_pattern_emit(prog, MX_PATTERN_OP_POSITION)->a = (uint8_t)prog->ncap;
            closed[prog->ncap] = true;
            prog->ncap += 1;
            p += 2;
        } else if (c == '(') {
            mx_pattern_emit(prog, MX_PATTERN_OP_OPEN)->a = (uint8_t)prog->ncap;
            closed[prog->ncap] = false;
            open[nopen] = (uint8_t)prog->ncap;
            nopen += 1;
            prog->ncap += 1;
            p += 1;
        } else if (c == ')' && nopen == 0) {
            mx_pattern_emit_fail(prog, "invalid pattern capture", 0);
            is_failed = true;
        } else if (c == ')') {
            nopen -= 1;
            mx_pattern_emit(prog, MX_PATTERN_OP_CLOSE)->a = open[nopen];
            closed[open[nopen]] = true;
            p += 1;
        } else if (c == '$' && p + 1 == plen) {
            mx_pattern_emit(prog, MX_PATTERN_OP_END);
            p += 1;
        } else if (c == '%' && p + 1 < plen && pat[p + 1] == 'b') {
            if (plen - p < 4) {
                mx_pattern_emit_fail(prog, "malformed pattern (missing arguments to '%%b')", 0);
                is_failed = true;
            } else {
                mx_pattern_item_t *item = mx_pattern_emit(prog, MX_PATTERN_OP_BALANCE);
                item->a = (uint8_t)pat[p + 2];
                item->b = (uint8_t)pat[p + 3];
                p += 4;
            }
        } else if (c == '%' && p + 1 < plen && pat[p + 1] == 'f') {
            if (p + 2 >= plen || pat[p + 2] != '[') {
                mx_pattern_emit_fail(prog, "missing '[' after '%%f' in pattern", 0);
                is_failed = true;
            } else {
                mx_pattern_item_t *item = mx_pattern_emit(prog, MX_PATTERN_OP_FRONTIER);
                mx_pattern_set_end_t end = mx_pattern_set_parse(cache, item->set, pat, plen, p + 2);
                if (!end.is_closed) {
                    prog->len -= 1;
                    mx_pattern_emit_fail(prog, "malformed pattern (missing ']')", 0);
                    is_failed = true;
                }
                p = end.next;
            }
        } else if (c == '%' && p + 1 < plen && isdigit((uint8_t)pat[p + 1]) != 0) {
            int32_t n = pat[p + 1] - '1';
            if (n < 0 || (uint32_t)n >= prog->ncap || !closed[n]) {
                mx_pattern_emit_fail(prog, "invalid capture index %%%d", (uint8_t)(n + 1));
                is_failed = true;
            } else {
                mx_pattern_emit(prog, MX_PATTERN_OP_BACKREF)->a = (uint8_t)n;
                p += 2;
            }
        } else {
            mx_pattern_item_t *item = mx_pattern_emit(prog, MX_PATTERN_OP_ONE);
            size_t next;
            if (c == '.') {
                for (size_t w = 0; w < 4; w++) {
                    item->set[w] = UINT64_MAX;
                }
                next = p + 1;
            } else if (c == '%' && p + 1 >= plen) {
                prog->len -= 1;
                mx_pattern_emit_fail(prog, "malformed pattern (ends with '%%')", 0);
                is_failed = true;
                next = plen;
            } else if (c == '%') {
                mx_pattern_set_escape(cache, item->set, (uint8_t)pat[p + 1]);
                next = p + 2;
            } else if (c == '[') {
                mx_pattern_set_end_t end = mx_pattern_set_parse(cache, item->set, pat, plen, p);
                if (!end.is_closed) {
                    prog->len -= 1;
                    mx_pattern_emit_fail(prog, "malformed pattern (missing ']')", 0);
                    is_failed = true;
                }
                next = end.next;
            } else {
                mx_pattern_set_add(item->set, (uint8_t)c);
                if (prog->len == 1) {
                    prog->first_byte = (uint8_t)c;
                }
                next = p + 1;
            }
            if (!is_failed && next < plen) {
                switch (pat[next]) {
                    case '?': {
                        item->rep = MX_PATTERN_REP_OPT;
                        next += 1;
                        break;
                    }
                    case '*': {
                        item->rep = MX_PATTERN_REP_STAR;
                        next += 1;
                        break;
                    }
                    case '+': {
                        item->rep = MX_PATTERN_REP_PLUS;
                        next += 1;
                        break;
                    }
                    case '-': {
                        item->rep = MX_PATTERN_REP_LAZY;
                        next += 1;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            if (item->rep != MX_PATTERN_REP_ONE && item->rep != MX_PATTERN_REP_PLUS) {
                prog->first_byte = -1;
            }
            p = next;
        }
    }
    if (!is_failed) {
        mx_pattern_emit(prog, MX_PATTERN_OP_DONE);
    }
}

const mx_pattern_prog_t *mx_pattern_prog(mx_pattern_cache_t *cache, mx_pattern_prog_t *own, const char *pat, size_t plen) {
    if (plen >= MX_PATTERN_ITEMS_INLINE) {
        mx_pattern_compile(cache, own, pat, plen);
        return own;
    }
    uint32_t h = MX_VM_FNV_OFFSET;
    for (size_t i = 0; i < plen; i++) {
        h = (h ^ (uint8_t)pat[i]) * MX_VM_FNV_PRIME;
    }
    mx_pattern_cached_t *e = &cache->progs[h & (MX_PATTERN_CACHE_WAYS - 1)];
    if (e->prog.items == NULL || e->len != plen || memcmp(e->pat, pat, plen) != 0) {
        memcpy(e->pat, pat, plen);
        e->len = plen;
        mx_pattern_compile(cache, &e->prog, pat, plen);
    }
    return &e->prog;
}

// For callers that run Lua while matching: a callback can recompile the cache
// slot that mx_pattern_prog returned, so they take a private copy.
void mx_pattern_prog_own(mx_pattern_cache_t *cache, mx_pattern_prog_t *own, const char *pat, size_t plen) {
    const mx_pattern_prog_t *prog = mx_pattern_prog(cache, own, pat, plen);
    if (prog != own) {
        own->items = own->inline_items;
        own->len = prog->len;
        own->ncap = prog->ncap;
        own->first_byte = prog->first_byte;
        memcpy(own->inline_items, prog->items, sizeof(mx_pattern_item_t) * prog->len);
    }
}

void mx_pattern_prog_free(mx_pattern_prog_t *prog) {
    if (prog->items != prog->inline_items) {
        mx_mem_free(prog->items);
    }
}

void mx_pattern_save(mx_pattern_run_t *r, uint32_t *nundo, uint32_t idx) {
    r->undos[*nundo] = (mx_pattern_undo_t) { .saved = r->caps[idx], .idx = idx };
    *nundo += 1;
}

int64_t mx_pattern_exec(mx_pattern_run_t *r, size_t start) {
    const mx_pattern_item_t *items = r->prog->items;
    const uint8_t *src = r->src;
    size_t len = r->len;
    uint32_t nchoice = 0;
    uint32_t nundo = 0;
    uint32_t i = 0;
    size_t s = start;
    while (true) {
        const mx_pattern_item_t *item = &items[i];
        bool ok = true;
        switch (item->op) {
            case MX_PATTERN_OP_ONE: {
                switch (item->rep) {
                    case MX_PATTERN_REP_ONE: {
                        ok = s < len && mx_pattern_set_has(item->set, src[s]);
                        s += 1;
                        break;
                    }
                    case MX_PATTERN_REP_OPT: {
                        if (s < len && mx_pattern_set_has(item->set, src[s])) {
                            r->choices[nchoice] = (mx_pattern_choice_t) {
                                .item = i,
                                .at = (uint32_t)s,
                                .undo = nundo,
                                .kind = MX_PATTERN_CHOICE_SKIP,
                            };
                            nchoice += 1;
                            s += 1;
                        }
                        break;
                    }
                    case MX_PATTERN_REP_STAR:
                    case MX_PATTERN_REP_PLUS: {
                        size_t lo = s;
                        if (item->rep == MX_PATTERN_REP_PLUS) {
                            lo += 1;
                        }
                        while (s < len && mx_pattern_set_has(item->set, src[s])) {
                            s += 1;
                        }
                        ok = s >= lo;
                        if (s > lo) {
                            r->choices[nchoice] = (mx_pattern_choice_t) {
                                .item = i,
                                .lo = (uint32_t)lo,
                                .at = (uint32_t)s,
                                .undo = nundo,
                                .kind = MX_PATTERN_CHOICE_GREEDY,
                            };
                            nchoice += 1;
                        }
                        break;
                    }
                    default: {
                        r->choices[nchoice] = (mx_pattern_choice_t) {
                            .item = i,
                            .at = (uint32_t)s,
                            .undo = nundo,
                            .kind = MX_PATTERN_CHOICE_LAZY,
                        };
                        nchoice += 1;
                        break;
                    }
                }
                break;
            }
            case MX_PATTERN_OP_OPEN: {
                mx_pattern_save(r, &nundo, item->a);
                r->caps[item->a] = (mx_pattern_capture_t) { .start = (uint32_t)s };
                break;
            }
            case MX_PATTERN_OP_POSITION: {
                mx_pattern_save(r, &nundo, item->a);
                r->caps[item->a] = (mx_pattern_capture_t) { .start = (uint32_t)s, .is_position = true, .is_closed = true };
                break;
            }
            case MX_PATTERN_OP_CLOSE: {
                mx_pattern_save(r, &nundo, item->a);
                r->caps[item->a].len = (uint32_t)s - r->caps[item->a].start;
                r->caps[item->a].is_closed = true;
                break;
            }
            case MX_PATTERN_OP_BACKREF: {
                const mx_pattern_capture_t *cap = &r->caps[item->a];
                ok = !cap->is_position && len - s >= cap->len && memcmp(src + cap->start, src + s, cap->len) == 0;
                s += cap->len;
                break;
            }
            case MX_PATTERN_OP_BALANCE: {
                ok = false;
                if (s < len && src[s] == item->a) {
                    uint32_t depth = 1;
                    size_t at = s + 1;
                    while (at < len && !ok) {
                        if (src[at] == item->b) {
                            depth -= 1;
                            ok = depth == 0;
                        } else if (src[at] == item->a) {
                            depth += 1;
                        }
                        at += 1;
                    }
                    s = at;
                }
                break;
            }
            case MX_PATTERN_OP_FRONTIER: {
                uint8_t before = 0;
                if (s > 0) {
                    before = src[s - 1];
                }
                uint8_t here = 0;
                if (s < len) {
                    here = src[s];
                }
                ok = !mx_pattern_set_has(item->set, before) && mx_pattern_set_has(item->set, here);
                break;
            }
            case MX_PATTERN_OP_END: {
                ok = s == len;
                break;
            }
            case MX_PATTERN_OP_FAIL: {
                r->error = item->error;
                r->error_arg = item->a;
                return -1;
            }
            default: {
                return (int64_t)s;
            }
        }
        if (nchoice + nundo > MX_PATTERN_MAX_DEPTH) {
            r->error = "pattern too complex";
            return -1;
        }
        i += 1;
        while (!ok) {
            if (nchoice == 0) {
                return -1;
            }
            mx_pattern_choice_t *top = &r->choices[nchoice - 1];
            while (nundo > top->undo) {
                nundo -= 1;
                r->caps[r->undos[nundo].idx] = r->undos[nundo].saved;
            }
            const uint64_t *set = items[top->item].set;
            switch (top->kind) {
                case MX_PATTERN_CHOICE_SKIP: {
                    s = top->at;
                    nchoice -= 1;
                    ok = true;
                    break;
                }
                case MX_PATTERN_CHOICE_GREEDY: {
                    top->at -= 1;
                    s = top->at;
                    if (top->at == top->lo) {
                        nchoice -= 1;
                    }
                    ok = true;
                    break;
                }
                default: {
                    if (top->at < len && mx_pattern_set_has(set, src[top->at])) {
                        top->at += 1;
                        s = top->at;
                        ok = true;
                    } else {
                        nchoice -= 1;
                    }
                    break;
                }
            }
            i = top->item + 1;
        }
    }
}

mx_pattern_result_t mx_pattern_search(mx_pattern_captures_t *scratch, const mx_pattern_prog_t *prog, const char *src, size_t src_len, size_t init, bool anchored) {
    mx_pattern_choice_t choices_buf[MX_PATTERN_ITEMS_INLINE];
    mx_pattern_undo_t undos_buf[MX_PATTERN_ITEMS_INLINE];
    mx_pattern_run_t r = {
        .prog = prog,
        .src = (const uint8_t *)src,
        .len = src_len,
        .caps = scratch->at,
        .choices = choices_buf,
        .undos = undos_buf,
    };
    if (prog->len > MX_PATTERN_ITEMS_INLINE) {
        r.choices = mx_mem_alloc(sizeof(mx_pattern_choice_t) * prog->len);
        r.undos = mx_mem_alloc(sizeof(mx_pattern_undo_t) * prog->len);
    }
    mx_pattern_result_t res = { .status = MX_PATTERN_NOMATCH };
    size_t s = init;
    bool is_searching = true;
    while (is_searching) {
        if (prog->first_byte >= 0 && !anchored && s < src_len) {
            const char *hit = memchr(src + s, prog->first_byte, src_len - s);
            if (hit == NULL) {
                s = src_len;
            } else {
                s = (size_t)(hit - src);
            }
        }
        int64_t stop = mx_pattern_exec(&r, s);
        if (r.error != NULL) {
            snprintf(scratch->error, MX_PATTERN_ERROR_CAP, r.error, r.error_arg);
            res = (mx_pattern_result_t) { .status = MX_PATTERN_ERROR, .error = scratch->error };
            is_searching = false;
        } else if (stop >= 0) {
            bool is_unfinished = false;
            for (uint32_t c = 0; c < prog->ncap; c++) {
                is_unfinished = is_unfinished || !scratch->at[c].is_closed;
            }
            res = (mx_pattern_result_t) {
                .status = MX_PATTERN_MATCHED,
                .match = {
                    .captures = scratch->at,
                    .start = (uint32_t)s,
                    .stop = (uint32_t)stop,
                    .len = prog->ncap,
                    .is_unfinished = is_unfinished,
                },
            };
            is_searching = false;
        } else {
            is_searching = !anchored && s < src_len;
            s += 1;
        }
    }
    if (r.choices != choices_buf) {
        mx_mem_free(r.choices);
        mx_mem_free(r.undos);
    }
    return res;
}

mx_pattern_result_t mx_pattern_match_from(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init, bool anchored) {
    mx_pattern_prog_t own;
    const mx_pattern_prog_t *prog;
    if (anchored) {
        prog = mx_pattern_prog(cache, &own, pat + 1, pat_len - 1);
    } else {
        prog = mx_pattern_prog(cache, &own, pat, pat_len);
    }
    mx_pattern_result_t res = mx_pattern_search(scratch, prog, src, src_len, init, anchored);
    if (prog == &own) {
        mx_pattern_prog_free(&own);
    }
    return res;
}

mx_pattern_result_t mx_pattern_match(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init) {
    return mx_pattern_match_from(cache, scratch, src, src_len, pat, pat_len, init, pat_len > 0 && pat[0] == '^');
}

mx_pattern_result_t mx_pattern_match_unanchored(mx_pattern_cache_t *cache, mx_pattern_captures_t *scratch, const char *src, size_t src_len, const char *pat, size_t pat_len, size_t init) {
    return mx_pattern_match_from(cache, scratch, src, src_len, pat, pat_len, init, false);
}
