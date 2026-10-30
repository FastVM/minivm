
#include "../api/std.h"
#include "../api/std_luaapi.h"
#include "../lua.h"
#include "../../front/chunk.h"
#include "../../front/dump.h"
#include "../../../value/error.h"
#include "../../../value/obj.h"
#include "../../../value/string.h"
#include "../../../value/table.h"

#include <dlfcn.h>

mx_std_text_t mx_std_read_stream(FILE *file) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = mx_mem_alloc(cap);
    while (true) {
        if (len == cap) {
            cap *= 2;
            buf = mx_mem_realloc(buf, cap);
        }
        size_t got = fread(buf + len, 1, cap - len, file);
        len += got;
        if (got == 0) {
            break;
        }
    }
    if (ferror(file) != 0) {
        mx_mem_free(buf);
        return (mx_std_text_t) { .ptr = NULL, .len = 0 };
    } else {
        return (mx_std_text_t) { .ptr = buf, .len = len };
    }
}

mx_std_text_t mx_std_read_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return (mx_std_text_t) { .ptr = NULL, .len = 0 };
    }
    mx_std_text_t text = mx_std_read_stream(file);
    fclose(file);
    return text;
}

mx_obj_t mx_std_load_closure(mx_t *vm, mx_ir_block_t *entry, mx_obj_t env) {
    uint32_t n = entry->func->nupvals;
    mx_obj_closure_t *cl = mx_obj_closure_new(vm, entry, n);
    for (uint32_t i = 0; i < n; i++) {
        mx_obj_t value;
        if (i == 0) {
            value = env;
        } else {
            value = mx_obj_of_nil();
        }
        cl->upvals[i] = mx_obj_upval_new_closed(vm, value);
    }
    return mx_obj_of_closure(cl);
}

int32_t mx_std_load_chunk(mx_std_state_t *ls, const char *src, size_t len, const char *name, const char *mode, mx_obj_t env) {
    mx_t *vm = ls->vm;
    bool is_binary = len > 0 && src[0] == MX_DUMP_MAGIC[0];
    const char *kind;
    if (is_binary) {
        kind = "binary";
    } else {
        kind = "text";
    }
    if (strchr(mode, kind[0]) == NULL) {
        mx_std_pushnil(ls);
        mx_std_pushfstring(ls, "attempt to load a %s chunk (mode is '%s')", kind, mode);
        return 2;
    } else if (len > UINT32_MAX - 1) {
        mx_std_pushnil(ls);
        mx_std_pushstring(ls, "chunk too large");
        return 2;
    }
    mx_ir_block_t *entry;
    if (is_binary) {
        mx_dump_loaded_t got = mx_dump_load(vm, src, len);
        if (got.error != NULL) {
            const char *shown;
            if (name[0] == '@' || name[0] == '=') {
                shown = name + 1;
            } else if (name[0] == MX_DUMP_MAGIC[0]) {
                shown = "binary string";
            } else {
                shown = name;
            }
            mx_std_pushnil(ls);
            mx_std_pushfstring(ls, "%s: bad binary format (%s)", shown, got.error);
            return 2;
        }
        entry = got.entry;
    } else {
        mx_chunk_compiled_t got = mx_chunk_compile(vm, src, (uint32_t)len, name);
        if (got.error != NULL) {
            mx_std_pushnil(ls);
            mx_std_push(ls, mx_error_payload(mx_obj_of_error(got.error)));
            return 2;
        }
        entry = got.entry;
    }
    mx_std_push(ls, mx_std_load_closure(vm, entry, env));
    return 1;
}

int32_t mx_std_load_file(mx_std_state_t *ls, const char *fname, const char *mode, mx_obj_t env) {
    mx_std_text_t text;
    mx_str_t *name = mx_str_new();
    if (fname == NULL) {
        text = mx_std_read_stream(stdin);
        mx_str_format(name, "=stdin");
    } else {
        text = mx_std_read_file(fname);
        mx_str_format(name, "@%s", fname);
    }
    if (text.ptr == NULL) {
        int32_t err = errno;
        mx_std_pushnil(ls);
        if (fname == NULL) {
            mx_std_pushfstring(ls, "cannot read stdin: %s", strerror(err));
        } else {
            mx_std_pushfstring(ls, "cannot open %s: %s", fname, strerror(err));
        }
        mx_str_free(name);
        return 2;
    }
    char *start;
    size_t avail;
    if (text.len >= 3 && memcmp(text.ptr, "\xEF\xBB\xBF", 3) == 0) {
        start = text.ptr + 3;
        avail = text.len - 3;
    } else {
        start = text.ptr;
        avail = text.len;
    }
    char *body;
    size_t len;
    if (avail > 0 && start[0] == '#') {
        size_t eol = 0;
        while (eol < avail && start[eol] != '\n') {
            eol += 1;
        }
        if (eol + 1 < avail && start[eol + 1] == MX_DUMP_MAGIC[0]) {
            body = start + eol + 1;
            len = avail - eol - 1;
        } else {
            memset(start, ' ', eol);
            body = start;
            len = avail;
        }
    } else {
        body = start;
        len = avail;
    }
    int32_t n = mx_std_load_chunk(ls, body, len, name->buf, mode, env);
    mx_mem_free(text.ptr);
    mx_str_free(name);
    return n;
}

int32_t mx_std_base_load(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    mx_obj_t env;
    if (!mx_std_isnone(ls, 4)) {
        env = *mx_std_at(ls, 4);
    } else {
        env = mx_lua_of(vm)->std;
    }
    const char *mode = mx_std_optstring(ls, 3, "bt").ptr;
    if (mode != NULL && strchr(mode, 'B') != NULL) {
        return mx_std_argerror(ls, 3, "invalid mode");
    } else if (mx_std_isstring(ls, 1)) {
        mx_std_str_t s = mx_std_tostring(ls, 1);
        const char *name = mx_std_optstring(ls, 2, s.ptr).ptr;
        if (vm->std_error.is_set) {
            return 0;
        }
        return mx_std_load_chunk(ls, s.ptr, s.len, name, mode, env);
    }
    mx_std_checktype(ls, 1, MX_LUA_TYPE_FUNCTION);
    const char *name = mx_std_optstring(ls, 2, "=(load)").ptr;
    if (vm->std_error.is_set) {
        return 0;
    }
    mx_str_t *buf = mx_str_new();
    while (true) {
        mx_std_pushvalue(ls, 1);
        if (mx_std_pcall(ls, 0, 1, 0) != MX_STD_OK) {
            mx_str_free(buf);
            mx_std_pushnil(ls);
            mx_std_insert(ls, -2);
            return 2;
        } else if (mx_std_isnil(ls, -1)) {
            mx_std_pop(ls, 1);
            break;
        } else if (!mx_std_isstring(ls, -1)) {
            mx_str_free(buf);
            mx_std_pushnil(ls);
            mx_std_pushstring(ls, "reader function must return a string");
            return 2;
        }
        mx_std_str_t piece = mx_std_tostring(ls, -1);
        mx_str_append(buf, piece.ptr, (uint32_t)piece.len);
        mx_std_pop(ls, 1);
        if (piece.len == 0) {
            break;
        }
    }
    int32_t n = mx_std_load_chunk(ls, buf->buf, buf->len, name, mode, env);
    mx_str_free(buf);
    return n;
}

int32_t mx_std_base_loadfile(mx_std_state_t *ls) {
    const char *fname = mx_std_optstring(ls, 1, NULL).ptr;
    const char *mode = mx_std_optstring(ls, 2, "bt").ptr;
    mx_obj_t env;
    if (!mx_std_isnone(ls, 3)) {
        env = *mx_std_at(ls, 3);
    } else {
        env = mx_lua_of(ls->vm)->std;
    }
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    return mx_std_load_file(ls, fname, mode, env);
}

int32_t mx_std_base_dofile(mx_std_state_t *ls) {
    mx_t *vm = ls->vm;
    const char *fname = mx_std_optstring(ls, 1, NULL).ptr;
    if (vm->std_error.is_set) {
        return 0;
    }
    mx_std_settop(ls, 1);
    if (mx_std_load_file(ls, fname, "bt", mx_lua_of(vm)->std) != 1) {
        ls->error = mx_error_value(vm, *mx_std_at(ls, -1));
        mx_std_raise(ls);
        return 0;
    } else if (ls->can_pend) {
        mx_call_pend(vm, MX_RUN_PENDING_KIND_CALL, *mx_std_at(ls, -1), mx_obj_of_nil(), NULL, 0, NULL);
        return 0;
    }
    mx_std_call(ls, 0, MX_STD_MULTRET);
    return ls->top - 1;
}

bool mx_std_package_readable(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        return false;
    } else {
        fclose(f);
        return true;
    }
}

int32_t mx_std_package_searchpath_aux(mx_std_state_t *ls, const char *name, const char *path, const char *sep, const char *dirsep) {
    mx_str_t *nm = mx_str_new();
    size_t seplen = strlen(sep);
    const char *s = name;
    while (*s != '\0') {
        if (seplen != 0 && strncmp(s, sep, seplen) == 0) {
            mx_str_append(nm, dirsep, (uint32_t)strlen(dirsep));
            s += seplen;
        } else {
            mx_str_append_byte(nm, *s);
            s += 1;
        }
    }
    mx_str_t *msg = mx_str_new();
    mx_str_t *file = mx_str_new();
    const char *p = path;
    bool is_found = false;
    while (*p != '\0' && !is_found) {
        const char *semi = strchr(p, ';');
        const char *end;
        if (semi == NULL) {
            end = p + strlen(p);
        } else {
            end = semi;
        }
        file->len = 0;
        for (const char *c = p; c < end; c++) {
            if (*c == '?') {
                mx_str_append(file, nm->buf, nm->len);
            } else {
                mx_str_append_byte(file, *c);
            }
        }
        if (end != p && mx_std_package_readable(file->buf)) {
            is_found = true;
        }
        if (*end == ';') {
            p = end + 1;
        } else {
            p = end;
        }
    }
    if (!is_found) {
        mx_str_format(msg, "no file '");
        for (const char *c = path; *c != '\0'; c++) {
            if (*c == ';') {
                mx_str_format(msg, "'\n\tno file '");
            } else if (*c == '?') {
                mx_str_append(msg, nm->buf, nm->len);
            } else {
                mx_str_append_byte(msg, *c);
            }
        }
        mx_str_format(msg, "'");
    }
    int32_t n;
    if (is_found) {
        mx_std_pushlstring(ls, file->buf, file->len);
        n = 1;
    } else {
        mx_std_pushnil(ls);
        mx_std_pushlstring(ls, msg->buf, msg->len);
        n = 2;
    }
    mx_str_free(file);
    mx_str_free(msg);
    mx_str_free(nm);
    return n;
}

int32_t mx_std_package_searchpath(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    const char *path = mx_std_checkstring(ls, 2).ptr;
    const char *sep = mx_std_optstring(ls, 3, ".").ptr;
    const char *dirsep = mx_std_optstring(ls, 4, "/").ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    return mx_std_package_searchpath_aux(ls, name, path, sep, dirsep);
}

mx_std_libstat_t mx_std_package_lookforfunc(mx_std_state_t *ls, const char *path, const char *sym) {
    int32_t scope;
    if (sym[0] == '*') {
        scope = RTLD_GLOBAL;
    } else {
        scope = RTLD_LOCAL;
    }
    void *lib = dlopen(path, RTLD_NOW | scope);
    if (lib == NULL) {
        mx_std_pushstring(ls, dlerror());
        return MX_STD_LIBSTAT_LIB;
    } else if (sym[0] == '*') {
        mx_std_pushboolean(ls, true);
        return MX_STD_LIBSTAT_OK;
    }
    mx_std_fn_t *fn = (mx_std_fn_t *)dlsym(lib, sym);
    if (fn == NULL) {
        mx_std_pushstring(ls, dlerror());
        return MX_STD_LIBSTAT_FUNC;
    }
    mx_luaapi_push_cfn(ls, fn, 0);
    return MX_STD_LIBSTAT_OK;
}

mx_std_libstat_t mx_std_package_loadfunc(mx_std_state_t *ls, const char *filename, const char *modname) {
    mx_str_t *name = mx_str_new();
    for (const char *p = modname; *p != '\0'; p++) {
        if (*p == '.') {
            mx_str_append_byte(name, '_');
        } else {
            mx_str_append_byte(name, *p);
        }
    }
    const char *mark = strchr(name->buf, '-');
    mx_str_t *sym = mx_str_new();
    mx_std_libstat_t st;
    if (mark == NULL) {
        mx_str_format(sym, "luaopen_%s", name->buf);
        st = mx_std_package_lookforfunc(ls, filename, sym->buf);
    } else {
        mx_str_format(sym, "luaopen_%.*s", (int32_t)(mark - name->buf), name->buf);
        mx_std_libstat_t first = mx_std_package_lookforfunc(ls, filename, sym->buf);
        if (first == MX_STD_LIBSTAT_FUNC) {
            mx_std_pop(ls, 1);
            sym->len = 0;
            mx_str_format(sym, "luaopen_%s", mark + 1);
            st = mx_std_package_lookforfunc(ls, filename, sym->buf);
        } else {
            st = first;
        }
    }
    mx_str_free(sym);
    mx_str_free(name);
    return st;
}

int32_t mx_std_package_loadlib(mx_std_state_t *ls) {
    const char *path = mx_std_checkstring(ls, 1).ptr;
    const char *init = mx_std_checkstring(ls, 2).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_libstat_t st = mx_std_package_lookforfunc(ls, path, init);
    if (st == MX_STD_LIBSTAT_OK) {
        return 1;
    }
    mx_std_pushnil(ls);
    mx_std_insert(ls, -2);
    if (st == MX_STD_LIBSTAT_LIB) {
        mx_std_pushstring(ls, "open");
    } else {
        mx_std_pushstring(ls, "init");
    }
    return 3;
}

const char *mx_std_package_path(mx_std_state_t *ls, const char *field) {
    mx_std_push(ls, mx_std_upval(ls, 0));
    mx_std_getfield(ls, -1, field);
    if (!mx_std_isstring(ls, -1)) {
        mx_std_error(ls, "'package.%s' must be a string", field);
        return NULL;
    } else {
        return mx_std_tostring(ls, -1).ptr;
    }
}

int32_t mx_std_package_searcher_preload(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_push(ls, mx_std_upval(ls, 0));
    if (mx_std_getfield(ls, -1, "preload") != MX_LUA_TYPE_TABLE) {
        return mx_std_error(ls, "'package.preload' must be a table");
    } else if (mx_std_getfield(ls, -1, name) == MX_LUA_TYPE_NIL) {
        mx_std_pushfstring(ls, "no field package.preload['%s']", name);
        return 1;
    } else {
        mx_std_pushstring(ls, ":preload:");
        return 2;
    }
}

int32_t mx_std_package_searcher_lua(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    const char *path = mx_std_package_path(ls, "path");
    if (path == NULL) {
        return 0;
    } else if (mx_std_package_searchpath_aux(ls, name, path, ".", "/") == 2) {
        return 1;
    }
    const char *filename = mx_std_tostring(ls, -1).ptr;
    if (mx_std_load_file(ls, filename, "bt", mx_lua_of(ls->vm)->std) != 1) {
        return mx_std_error(ls, "error loading module '%s' from file '%s':\n\t%s", name, filename, mx_std_tostring(ls, -1).ptr);
    }
    mx_std_pushstring(ls, filename);
    return 2;
}

int32_t mx_std_package_searcher_c(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    const char *path = mx_std_package_path(ls, "cpath");
    if (path == NULL) {
        return 0;
    } else if (mx_std_package_searchpath_aux(ls, name, path, ".", "/") == 2) {
        return 1;
    }
    const char *filename = mx_std_tostring(ls, -1).ptr;
    if (mx_std_package_loadfunc(ls, filename, name) != MX_STD_LIBSTAT_OK) {
        return mx_std_error(ls, "error loading module '%s' from file '%s':\n\t%s", name, filename, mx_std_tostring(ls, -1).ptr);
    }
    mx_std_pushstring(ls, filename);
    return 2;
}

int32_t mx_std_package_searcher_croot(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    const char *dot = strchr(name, '.');
    if (dot == NULL) {
        return 0;
    }
    const char *path = mx_std_package_path(ls, "cpath");
    if (path == NULL) {
        return 0;
    }
    mx_std_pushlstring(ls, name, (size_t)(dot - name));
    if (mx_std_package_searchpath_aux(ls, mx_std_tostring(ls, -1).ptr, path, ".", "/") == 2) {
        return 1;
    }
    const char *filename = mx_std_tostring(ls, -1).ptr;
    mx_std_libstat_t st = mx_std_package_loadfunc(ls, filename, name);
    if (st == MX_STD_LIBSTAT_LIB) {
        return mx_std_error(ls, "error loading module '%s' from file '%s':\n\t%s", name, filename, mx_std_tostring(ls, -1).ptr);
    } else if (st == MX_STD_LIBSTAT_FUNC) {
        mx_std_pushfstring(ls, "no module '%s' in file '%s'", name, filename);
        return 1;
    }
    mx_std_pushstring(ls, filename);
    return 2;
}

int32_t mx_std_package_require(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_settop(ls, 1);
    mx_std_getfield(ls, MX_STD_REGISTRYINDEX, "_LOADED");
    mx_std_getfield(ls, 2, name);
    if (mx_std_toboolean(ls, 3)) {
        return 1;
    }
    mx_std_pop(ls, 1);
    mx_std_push(ls, mx_std_upval(ls, 0));
    if (mx_std_getfield(ls, 3, "searchers") != MX_LUA_TYPE_TABLE) {
        return mx_std_error(ls, "'package.searchers' must be a table");
    }
    mx_str_t *msg = mx_str_new();
    for (int64_t i = 1; true; i++) {
        if (mx_std_rawgeti(ls, 4, i) == MX_LUA_TYPE_NIL) {
            mx_std_error(ls, "module '%s' not found:%s", name, msg->buf);
            mx_str_free(msg);
            return 0;
        }
        mx_std_pushstring(ls, name);
        mx_std_call(ls, 1, 2);
        if (ls->vm->std_error.is_set) {
            mx_str_free(msg);
            return 0;
        } else if (mx_std_isfunction(ls, 5)) {
            break;
        } else if (mx_std_isstring(ls, 5)) {
            mx_std_str_t s = mx_std_tostring(ls, 5);
            mx_str_format(msg, "\n\t");
            mx_str_append(msg, s.ptr, (uint32_t)s.len);
        }
        mx_std_pop(ls, 2);
    }
    mx_str_free(msg);
    mx_std_pushvalue(ls, 5);
    mx_std_pushvalue(ls, 1);
    mx_std_pushvalue(ls, 6);
    mx_std_call(ls, 2, 1);
    if (ls->vm->std_error.is_set) {
        return 0;
    } else if (!mx_std_isnil(ls, 7)) {
        mx_std_pushvalue(ls, 7);
        mx_std_setfield(ls, 2, name);
    }
    if (mx_std_getfield(ls, 2, name) == MX_LUA_TYPE_NIL) {
        mx_std_pop(ls, 1);
        mx_std_pushboolean(ls, true);
        mx_std_pushvalue(ls, -1);
        mx_std_setfield(ls, 2, name);
    }
    mx_std_pushvalue(ls, 6);
    return 2;
}

const mx_std_reg_t mx_std_package_regs[] = {
    {"searchpath", mx_std_package_searchpath, NULL},
    {"loadlib", mx_std_package_loadlib, NULL},
    {NULL, NULL, NULL},
};

const mx_std_reg_t mx_std_load_regs[] = {
    {"load", mx_std_base_load, NULL},
    {"loadfile", mx_std_base_loadfile, NULL},
    {"dofile", mx_std_base_dofile, NULL},
    {NULL, NULL, NULL},
};

void mx_std_package_setpath(mx_std_state_t *ls, const char *field, const char *env, const char *dft) {
    mx_str_t *versioned = mx_str_new();
    mx_str_format(versioned, "%s_5_5", env);
    const char *exact = getenv(versioned->buf);
    mx_str_free(versioned);
    const char *path;
    if (exact == NULL) {
        path = getenv(env);
    } else {
        path = exact;
    }
    mx_std_getfield(ls, MX_STD_REGISTRYINDEX, "LUA_NOENV");
    bool is_noenv = mx_std_toboolean(ls, -1);
    mx_std_pop(ls, 1);
    const char *mark;
    if (path != NULL) {
        mark = strstr(path, ";;");
    } else {
        mark = NULL;
    }
    if (path == NULL || is_noenv) {
        mx_std_pushstring(ls, dft);
    } else if (mark == NULL) {
        mx_std_pushstring(ls, path);
    } else {
        const char *end = path + strlen(path);
        mx_std_buf_t buf;
        mx_std_buf_init(ls, &buf);
        if (mark > path) {
            mx_std_buf_addlstring(&buf, path, (size_t)(mark - path));
            mx_std_buf_addchar(&buf, ';');
        }
        mx_std_buf_addstring(&buf, dft);
        if (mark + 2 < end) {
            mx_std_buf_addchar(&buf, ';');
            mx_std_buf_addlstring(&buf, mark + 2, (size_t)(end - mark - 2));
        }
        mx_std_buf_push(&buf);
    }
    mx_std_setfield(ls, -2, field);
}

void mx_std_open_package(mx_std_state_t *ls) {
    mx_std_newlib(ls, "package", mx_std_package_regs);
    mx_std_fn_t *const searchers[] = {
        mx_std_package_searcher_preload,
        mx_std_package_searcher_lua,
        mx_std_package_searcher_c,
        mx_std_package_searcher_croot,
    };
    mx_std_newtable(ls);
    for (int64_t i = 0; i < 4; i++) {
        mx_std_pushvalue(ls, -2);
        mx_std_pushcfn_up(ls, searchers[i], "searcher", 1);
        mx_std_rawseti(ls, -2, i + 1);
    }
    mx_std_setfield(ls, -2, "searchers");
    mx_std_newtable(ls);
    mx_std_setfield(ls, -2, "preload");
    mx_std_package_setpath(ls, "path", "LUA_PATH", "/usr/local/share/lua/5.5/?.lua;/usr/local/share/lua/5.5/?/init.lua;/usr/local/lib/lua/5.5/?.lua;/usr/local/lib/lua/5.5/?/init.lua;./?.lua;./?/init.lua");
    mx_std_package_setpath(ls, "cpath", "LUA_CPATH", "/usr/local/lib/lua/5.5/?.so;/usr/local/lib/lua/5.5/loadall.so;./?.so");
    mx_std_pushstring(ls, "/\n;\n?\n!\n-\n");
    mx_std_setfield(ls, -2, "config");
    mx_std_newtable(ls);
    const char *const libs[] = {"_G", "package", "coroutine", "table", "io", "os", "string", "math", "utf8", "debug"};
    for (uint32_t i = 0; i < sizeof(libs) / sizeof(libs[0]); i++) {
        mx_std_getglobal(ls, libs[i]);
        mx_std_setfield(ls, -2, libs[i]);
    }
    mx_std_push(ls, mx_lua_of(ls->vm)->registry);
    mx_std_pushvalue(ls, -2);
    mx_std_setfield(ls, -2, "_LOADED");
    mx_std_pop(ls, 1);
    mx_std_setfield(ls, -2, "loaded");
    mx_std_push(ls, mx_lua_of(ls->vm)->std);
    mx_std_register(ls, mx_std_load_regs);
    mx_std_pushvalue(ls, -2);
    mx_std_pushcfn_up(ls, mx_std_package_require, "require", 1);
    mx_std_setfield(ls, -2, "require");
    mx_std_pop(ls, 2);
}
