
#include "../api/std.h"
#include "../lua.h"
#include "../../../vm/gc.h"
#include "../../../value/kind.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

enum {
    MX_STD_IO_VBUF_DEFAULT = 1024,
    MX_STD_IO_LINES_FIXED_UPS = 3,
    MX_STD_IO_LINES_MAX_FORMATS = 250,
    MX_STD_IO_READ_STACK_MARGIN = 20,
    MX_STD_IO_READ_CHUNK = 4096,
};

mx_obj_table_t *mx_std_io_table_of(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_t *at = mx_std_at(ls, slot);
    if (at == NULL || !mx_obj_is_table(*at)) {
        return NULL;
    } else {
        mx_obj_table_t *t = mx_obj_get_table(*at);
        if (t->head.kind != mx_lua_of(ls->vm)->file_kind) {
            return NULL;
        } else {
            return t;
        }
    }
}

mx_obj_table_t *mx_std_io_check_file(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_table_t *t = mx_std_io_table_of(ls, slot);
    if (t == NULL) {
        mx_std_typeerror(ls, slot, "FILE*");
        return NULL;
    } else {
        return t;
    }
}

bool mx_std_io_isopen_table(const mx_obj_table_t *t) {
    return (t->hidden & ~(uint64_t)1) != 0;
}

bool mx_std_io_ispipe_table(const mx_obj_table_t *t) {
    return (t->hidden & 1) != 0;
}

FILE *mx_std_io_file_of_table(const mx_obj_table_t *t) {
    return (FILE *)(uintptr_t)(t->hidden & ~(uint64_t)1);
}

FILE *mx_std_io_tofile(mx_std_state_t *ls, mx_std_slot_t slot) {
    mx_obj_table_t *t = mx_std_io_check_file(ls, slot);
    if (t == NULL) {
        return NULL;
    } else if (!mx_std_io_isopen_table(t)) {
        mx_std_error(ls, "attempt to use a closed file");
        return NULL;
    } else {
        return mx_std_io_file_of_table(t);
    }
}

void mx_std_io_new_handle(mx_std_state_t *ls, FILE *fp, bool is_pipe) {
    mx_lua_t *lua = mx_lua_of(ls->vm);
    mx_obj_table_t *t = mx_table_new(ls->vm);
    t->head.kind = lua->file_kind;
    t->is_plain = false;
    t->hidden = (uint64_t)(uintptr_t)fp | (uint64_t)is_pipe;
    t->meta = mx_obj_get_table(lua->file_meta);
    mx_std_push(ls, mx_obj_of_table(t));
}

int32_t mx_std_io_fileresult(mx_std_state_t *ls, bool ok, const char *fname) {
    int32_t en = errno;
    if (ok) {
        mx_std_pushboolean(ls, true);
        return 1;
    } else {
        mx_std_pushnil(ls);
        if (fname != NULL) {
            mx_std_pushfstring(ls, "%s: %s", fname, strerror(en));
        } else {
            mx_std_pushstring(ls, strerror(en));
        }
        mx_std_pushinteger(ls, en);
        return 3;
    }
}

bool mx_std_io_checkmode(const char *mode) {
    if (*mode != 'r' && *mode != 'w' && *mode != 'a') {
        return false;
    } else {
        const char *plus = mode + 1;
        const char *bin = plus + (*plus == '+');
        const char *end = bin + (*bin == 'b');
        return *end == '\0';
    }
}

int32_t mx_std_io_open(mx_std_state_t *ls) {
    const char *fname = mx_std_checkstring(ls, 1).ptr;
    const char *mode = mx_std_optstring(ls, 2, "r").ptr;
    if (!mx_std_io_checkmode(mode)) {
        return mx_std_argerror(ls, 2, "invalid mode");
    }
    FILE *fp = fopen(fname, mode);
    if (fp == NULL) {
        return mx_std_io_fileresult(ls, false, fname);
    }
    mx_std_io_new_handle(ls, fp, false);
    return 1;
}

int32_t mx_std_io_popen(mx_std_state_t *ls) {
    const char *cmd = mx_std_checkstring(ls, 1).ptr;
    const char *mode = mx_std_optstring(ls, 2, "r").ptr;
    if (!((mode[0] == 'r' || mode[0] == 'w') && mode[1] == '\0')) {
        return mx_std_argerror(ls, 2, "invalid mode");
    }
    fflush(NULL);
    FILE *fp = popen(cmd, mode);
    if (fp == NULL) {
        return mx_std_io_fileresult(ls, false, cmd);
    }
    mx_std_io_new_handle(ls, fp, true);
    return 1;
}

int32_t mx_std_io_tmpfile(mx_std_state_t *ls) {
    FILE *fp = tmpfile();
    if (fp == NULL) {
        return mx_std_io_fileresult(ls, false, NULL);
    }
    mx_std_io_new_handle(ls, fp, false);
    return 1;
}

int32_t mx_std_io_close_table(mx_std_state_t *ls, mx_obj_table_t *t) {
    if (t == NULL) {
        return 0;
    }
    FILE *fp = mx_std_io_file_of_table(t);
    if (fp == NULL) {
        return 0;
    } else if (fp == stdin || fp == stdout || fp == stderr) {
        mx_std_pushnil(ls);
        mx_std_pushstring(ls, "cannot close standard file");
        return 2;
    } else if (mx_std_io_ispipe_table(t)) {
        errno = 0;
        int32_t stat = pclose(fp);
        t->hidden = 0;
        return mx_std_os_execresult(ls, stat);
    } else {
        int32_t res = fclose(fp);
        t->hidden = 0;
        return mx_std_io_fileresult(ls, res == 0, NULL);
    }
}

int32_t mx_std_file_close(mx_std_state_t *ls) {
    mx_std_io_tofile(ls, 1);
    return mx_std_io_close_table(ls, mx_std_io_table_of(ls, 1));
}

int32_t mx_std_file_gc(mx_std_state_t *ls) {
    mx_obj_table_t *t = mx_std_io_check_file(ls, 1);
    if (t != NULL && mx_std_io_isopen_table(t)) {
        mx_std_io_close_table(ls, t);
    }
    return 0;
}

int32_t mx_std_io_close(mx_std_state_t *ls) {
    if (mx_std_isnone(ls, 1)) {
        mx_std_push(ls, mx_lua_of(ls->vm)->io_out);
    }
    mx_std_io_tofile(ls, 1);
    return mx_std_io_close_table(ls, mx_std_io_table_of(ls, 1));
}

int32_t mx_std_io_type(mx_std_state_t *ls) {
    mx_std_checkany(ls, 1);
    mx_obj_table_t *t = mx_std_io_table_of(ls, 1);
    if (t == NULL) {
        mx_std_pushnil(ls);
    } else if (!mx_std_io_isopen_table(t)) {
        mx_std_pushstring(ls, "closed file");
    } else {
        mx_std_pushstring(ls, "file");
    }
    return 1;
}

int32_t mx_std_file_tostring(mx_std_state_t *ls) {
    mx_obj_table_t *t = mx_std_io_check_file(ls, 1);
    if (t == NULL || !mx_std_io_isopen_table(t)) {
        mx_std_pushstring(ls, "file (closed)");
    } else {
        mx_std_pushfstring(ls, "file (%p)", (void *)mx_std_io_file_of_table(t));
    }
    return 1;
}

bool mx_std_io_num_take(mx_std_io_num_t *r, bool ok) {
    if (ok && r->n < MX_STD_IO_NUMBUF) {
        r->buf[r->n] = (char)r->c;
        r->n += 1;
        r->c = getc(r->fp);
        return true;
    } else {
        return false;
    }
}

bool mx_std_io_read_number(mx_std_state_t *ls, FILE *fp) {
    mx_std_io_num_t r = (mx_std_io_num_t) { .fp = fp, .c = getc(fp), .n = 0 };
    while (isspace(r.c) != 0) {
        r.c = getc(fp);
    }
    mx_std_io_num_take(&r, r.c == '-' || r.c == '+');
    bool hex = mx_std_io_num_take(&r, r.c == '0') && mx_std_io_num_take(&r, r.c == 'x' || r.c == 'X');
    bool more = true;
    while (more) {
        bool ok;
        if (hex) {
            ok = isxdigit(r.c) != 0 || r.c == '.' || r.c == 'p' || r.c == 'P' || r.c == '-' || r.c == '+';
        } else {
            ok = isdigit(r.c) != 0 || r.c == '.' || r.c == 'e' || r.c == 'E' || r.c == '-' || r.c == '+';
        }
        more = mx_std_io_num_take(&r, ok);
    }
    ungetc(r.c, fp);
    if (r.n == MX_STD_IO_NUMBUF) {
        mx_std_pushnil(ls);
        return false;
    } else {
        r.buf[r.n] = '\0';
        mx_obj_num_t num = mx_std_str2num(r.buf, r.n);
        if (num.is_num) {
            mx_std_push(ls, num.value);
            return true;
        } else {
            mx_std_pushnil(ls);
            return false;
        }
    }
}

bool mx_std_io_read_line(mx_std_state_t *ls, FILE *fp, bool chop) {
    mx_std_buf_t b;
    mx_std_buf_init(ls, &b);
    int32_t c = getc(fp);
    while (c != EOF && c != '\n') {
        mx_std_buf_addchar(&b, (char)c);
        c = getc(fp);
    }
    if (!chop && c == '\n') {
        mx_std_buf_addchar(&b, '\n');
    }
    bool ok = c == '\n' || b.len > 0;
    mx_std_buf_push(&b);
    return ok;
}

void mx_std_io_read_all(mx_std_state_t *ls, FILE *fp) {
    mx_std_buf_t b;
    mx_std_buf_init(ls, &b);
    size_t nr = MX_STD_IO_READ_CHUNK;
    while (nr == MX_STD_IO_READ_CHUNK) {
        char *p = mx_std_buf_prep(&b, MX_STD_IO_READ_CHUNK);
        if (p == NULL) {
            return;
        }
        nr = fread(p, 1, MX_STD_IO_READ_CHUNK, fp);
        b.len += nr;
    }
    mx_std_buf_push(&b);
}

bool mx_std_io_read_chars(mx_std_state_t *ls, FILE *fp, size_t n) {
    mx_std_buf_t b;
    mx_std_buf_init(ls, &b);
    char *p = mx_std_buf_prep(&b, n);
    if (p == NULL) {
        return false;
    }
    size_t nr = fread(p, 1, n, fp);
    b.len = nr;
    mx_std_buf_push(&b);
    return nr > 0;
}

bool mx_std_io_test_eof(mx_std_state_t *ls, FILE *fp) {
    int32_t c = getc(fp);
    ungetc(c, fp);
    mx_std_pushlstring(ls, "", 0);
    return c != EOF;
}

int32_t mx_std_io_read_aux(mx_std_state_t *ls, FILE *fp, int32_t first) {
    if (fp == NULL) {
        return 0;
    }
    int32_t nargs = mx_std_top(ls) - 1;
    clearerr(fp);
    int32_t n;
    bool success;
    if (nargs == 0) {
        success = mx_std_io_read_line(ls, fp, true);
        n = first + 1;
    } else {
        if (!mx_std_checkstack(ls, nargs + MX_STD_IO_READ_STACK_MARGIN)) {
            return mx_std_error(ls, "too many arguments");
        }
        success = true;
        for (n = first; n < first + nargs && success; n++) {
            if (mx_std_isinteger(ls, n)) {
                size_t l = (size_t)mx_std_checkinteger(ls, n);
                if (l == 0) {
                    success = mx_std_io_test_eof(ls, fp);
                } else {
                    success = mx_std_io_read_chars(ls, fp, l);
                }
            } else {
                const char *fmt = mx_std_checkstring(ls, n).ptr;
                char opt;
                if (*fmt == '*') {
                    opt = fmt[1];
                } else {
                    opt = fmt[0];
                }
                switch (opt) {
                    case 'n': {
                        success = mx_std_io_read_number(ls, fp);
                        break;
                    }
                    case 'l': {
                        success = mx_std_io_read_line(ls, fp, true);
                        break;
                    }
                    case 'L': {
                        success = mx_std_io_read_line(ls, fp, false);
                        break;
                    }
                    case 'a': {
                        mx_std_io_read_all(ls, fp);
                        success = true;
                        break;
                    }
                    default: {
                        return mx_std_argerror(ls, n, "invalid format");
                    }
                }
            }
        }
    }
    if (ferror(fp) != 0) {
        return mx_std_io_fileresult(ls, false, NULL);
    } else if (!success) {
        mx_std_pop(ls, 1);
        mx_std_pushnil(ls);
    }
    return n - first;
}

int32_t mx_std_io_read(mx_std_state_t *ls) {
    mx_std_push(ls, mx_lua_of(ls->vm)->io_in);
    FILE *fp = mx_std_io_tofile(ls, -1);
    return mx_std_io_read_aux(ls, fp, 1);
}

int32_t mx_std_file_read(mx_std_state_t *ls) {
    FILE *fp = mx_std_io_tofile(ls, 1);
    return mx_std_io_read_aux(ls, fp, 2);
}

int32_t mx_std_io_write_aux(mx_std_state_t *ls, FILE *fp, int32_t arg) {
    if (fp == NULL) {
        return 0;
    }
    int32_t top = mx_std_top(ls);
    bool status = true;
    for (int32_t i = arg; i < top && !ls->vm->std_error.is_set; i++) {
        mx_obj_t *at = mx_std_at(ls, i);
        if (at != NULL && mx_obj_is_number(*at)) {
            mx_str_t *sb = mx_str_new();
            if (mx_obj_is_int(*at)) {
                mx_str_append_int(sb, mx_obj_get_int(*at));
            } else {
                mx_str_float(sb, mx_obj_get_float(*at));
            }
            status = status && fwrite(sb->buf, 1, sb->len, fp) == sb->len;
            mx_str_free(sb);
        } else {
            mx_std_str_t s = mx_std_checkstring(ls, i);
            status = status && fwrite(s.ptr, 1, s.len, fp) == s.len;
        }
    }
    if (status) {
        return 1;
    } else {
        return mx_std_io_fileresult(ls, false, NULL);
    }
}

int32_t mx_std_io_write(mx_std_state_t *ls) {
    mx_std_push(ls, mx_lua_of(ls->vm)->io_out);
    FILE *fp = mx_std_io_tofile(ls, -1);
    return mx_std_io_write_aux(ls, fp, 1);
}

int32_t mx_std_file_write(mx_std_state_t *ls) {
    FILE *fp = mx_std_io_tofile(ls, 1);
    mx_std_pushvalue(ls, 1);
    return mx_std_io_write_aux(ls, fp, 2);
}

int32_t mx_std_file_seek(mx_std_state_t *ls) {
    const int32_t mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    const char *const modenames[] = {"set", "cur", "end", NULL};
    FILE *fp = mx_std_io_tofile(ls, 1);
    if (fp == NULL) {
        return 0;
    }
    int32_t op = mx_std_checkoption(ls, 2, "cur", modenames);
    int64_t offset = mx_std_optinteger(ls, 3, 0);
    if (fseek(fp, (long)offset, mode[op]) != 0) {
        return mx_std_io_fileresult(ls, false, NULL);
    } else {
        mx_std_pushinteger(ls, (int64_t)ftell(fp));
        return 1;
    }
}

int32_t mx_std_file_setvbuf(mx_std_state_t *ls) {
    const int32_t mode[] = {_IONBF, _IOFBF, _IOLBF};
    const char *const modenames[] = {"no", "full", "line", NULL};
    FILE *fp = mx_std_io_tofile(ls, 1);
    if (fp == NULL) {
        return 0;
    }
    int32_t op = mx_std_checkoption(ls, 2, NULL, modenames);
    int64_t sz = mx_std_optinteger(ls, 3, MX_STD_IO_VBUF_DEFAULT);
    int32_t res = setvbuf(fp, NULL, mode[op], (size_t)sz);
    return mx_std_io_fileresult(ls, res == 0, NULL);
}

int32_t mx_std_file_flush(mx_std_state_t *ls) {
    FILE *fp = mx_std_io_tofile(ls, 1);
    if (fp == NULL) {
        return 0;
    } else {
        return mx_std_io_fileresult(ls, fflush(fp) == 0, NULL);
    }
}

int32_t mx_std_io_flush(mx_std_state_t *ls) {
    mx_std_push(ls, mx_lua_of(ls->vm)->io_out);
    FILE *fp = mx_std_io_tofile(ls, -1);
    if (fp == NULL) {
        return 0;
    } else {
        return mx_std_io_fileresult(ls, fflush(fp) == 0, NULL);
    }
}

int32_t mx_std_io_iofile(mx_std_state_t *ls, mx_obj_t *slot, const char *mode) {
    if (!mx_std_isnoneornil(ls, 1)) {
        const char *fname = mx_std_tostring(ls, 1).ptr;
        if (fname != NULL) {
            FILE *fp = fopen(fname, mode);
            if (fp == NULL) {
                return mx_std_error(ls, "cannot open file '%s' (%s)", fname, strerror(errno));
            }
            mx_std_io_new_handle(ls, fp, false);
            *slot = *mx_std_at(ls, -1);
            mx_std_pop(ls, 1);
        } else if (mx_std_io_tofile(ls, 1) != NULL) {
            *slot = *mx_std_at(ls, 1);
        }
    }
    mx_std_push(ls, *slot);
    return 1;
}

int32_t mx_std_io_input(mx_std_state_t *ls) {
    return mx_std_io_iofile(ls, &mx_lua_of(ls->vm)->io_in, "r");
}

int32_t mx_std_io_output(mx_std_state_t *ls) {
    return mx_std_io_iofile(ls, &mx_lua_of(ls->vm)->io_out, "w");
}

int32_t mx_std_io_lines_iter(mx_std_state_t *ls) {
    mx_obj_t fobj = mx_std_upval(ls, 0);
    int32_t n = (int32_t)mx_obj_get_int(mx_std_upval(ls, 1));
    bool toclose = mx_obj_is_truthy(mx_std_upval(ls, 2));
    mx_std_settop(ls, 0);
    mx_std_push(ls, fobj);
    mx_obj_table_t *t = mx_std_io_table_of(ls, 1);
    if (t == NULL || !mx_std_io_isopen_table(t)) {
        return mx_std_error(ls, "file is already closed");
    }
    FILE *fp = mx_std_io_file_of_table(t);
    for (int32_t i = 0; i < n; i++) {
        mx_std_push(ls, mx_std_upval(ls, (uint32_t)(MX_STD_IO_LINES_FIXED_UPS + i)));
    }
    int32_t nres = mx_std_io_read_aux(ls, fp, 2);
    if (mx_std_toboolean(ls, -nres)) {
        return nres;
    } else if (nres > 1) {
        const char *msg = mx_std_tostring(ls, -nres + 1).ptr;
        if (msg == NULL) {
            return mx_std_error(ls, "read error");
        } else {
            return mx_std_error(ls, "%s", msg);
        }
    } else if (toclose) {
        mx_std_settop(ls, 0);
        mx_std_push(ls, fobj);
        mx_std_io_close_table(ls, t);
        return 0;
    } else {
        return 0;
    }
}

void mx_std_io_lines_aux(mx_std_state_t *ls, bool toclose) {
    int32_t n = mx_std_top(ls) - 1;
    if (n > MX_STD_IO_LINES_MAX_FORMATS) {
        mx_std_argerror(ls, MX_STD_IO_LINES_MAX_FORMATS + 2, "too many arguments");
    } else {
        mx_std_pushvalue(ls, 1);
        mx_std_pushinteger(ls, n);
        mx_std_pushboolean(ls, toclose);
        for (int32_t i = 1; i <= n; i++) {
            mx_std_pushvalue(ls, i + 1);
        }
        mx_std_pushcfn_up(ls, mx_std_io_lines_iter, "lines", (uint32_t)(MX_STD_IO_LINES_FIXED_UPS + n));
    }
}

int32_t mx_std_file_lines(mx_std_state_t *ls) {
    mx_std_io_tofile(ls, 1);
    mx_std_io_lines_aux(ls, false);
    return 1;
}

int32_t mx_std_io_lines(mx_std_state_t *ls) {
    bool toclose;
    if (mx_std_isnone(ls, 1)) {
        mx_std_pushnil(ls);
    }
    if (mx_std_isnil(ls, 1)) {
        mx_std_push(ls, mx_lua_of(ls->vm)->io_in);
        mx_std_replace(ls, 1);
        mx_std_io_tofile(ls, 1);
        toclose = false;
    } else {
        const char *fname = mx_std_checkstring(ls, 1).ptr;
        FILE *fp = fopen(fname, "r");
        if (fp == NULL) {
            return mx_std_error(ls, "cannot open file '%s' (%s)", fname, strerror(errno));
        }
        mx_std_io_new_handle(ls, fp, false);
        mx_std_replace(ls, 1);
        toclose = true;
    }
    mx_std_io_lines_aux(ls, toclose);
    if (toclose) {
        mx_std_pushnil(ls);
        mx_std_pushnil(ls);
        mx_std_pushvalue(ls, 1);
        return 4;
    } else {
        return 1;
    }
}

const mx_std_reg_t mx_std_file_regs[] = {
    {"read", mx_std_file_read, NULL},
    {"write", mx_std_file_write, NULL},
    {"lines", mx_std_file_lines, NULL},
    {"close", mx_std_file_close, NULL},
    {"seek", mx_std_file_seek, NULL},
    {"flush", mx_std_file_flush, NULL},
    {"setvbuf", mx_std_file_setvbuf, NULL},
    {NULL, NULL, NULL},
};

const mx_std_reg_t mx_std_io_regs[] = {
    {"write", mx_std_io_write, NULL},
    {"read", mx_std_io_read, NULL},
    {"open", mx_std_io_open, NULL},
    {"close", mx_std_io_close, NULL},
    {"lines", mx_std_io_lines, NULL},
    {"input", mx_std_io_input, NULL},
    {"output", mx_std_io_output, NULL},
    {"flush", mx_std_io_flush, NULL},
    {"type", mx_std_io_type, NULL},
    {"tmpfile", mx_std_io_tmpfile, NULL},
    {"popen", mx_std_io_popen, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_io(mx_std_state_t *ls) {
    mx_lua_t *lua = mx_lua_of(ls->vm);
    mx_std_newtable(ls);
    mx_std_newtable(ls);
    mx_std_register(ls, mx_std_file_regs);
    mx_std_setfield(ls, -2, "__index");
    mx_std_pushcfn(ls, mx_std_file_tostring, "__tostring");
    mx_std_setfield(ls, -2, "__tostring");
    mx_std_pushcfn(ls, mx_std_file_gc, "__gc");
    mx_std_setfield(ls, -2, "__gc");
    mx_std_pushcfn(ls, mx_std_file_gc, "__close");
    mx_std_setfield(ls, -2, "__close");
    mx_std_pushstring(ls, "FILE*");
    mx_std_setfield(ls, -2, "__name");
    lua->file_meta = *mx_std_at(ls, -1);
    mx_std_pop(ls, 1);
    mx_std_newlib(ls, "io", mx_std_io_regs);
    mx_std_io_new_handle(ls, stdin, false);
    lua->io_in = *mx_std_at(ls, -1);
    mx_std_setfield(ls, -2, "stdin");
    mx_std_io_new_handle(ls, stdout, false);
    lua->io_out = *mx_std_at(ls, -1);
    mx_std_setfield(ls, -2, "stdout");
    mx_std_io_new_handle(ls, stderr, false);
    mx_std_setfield(ls, -2, "stderr");
    mx_std_pop(ls, 1);
}
