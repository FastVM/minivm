
#include "../api/std.h"
#include "../lua.h"
#include "../../../value/obj.h"

int32_t mx_std_os_clock(mx_std_state_t *ls) {
    mx_std_pushfloat(ls, (double)clock() / (double)CLOCKS_PER_SEC);
    return 1;
}

int32_t mx_std_os_getfield(mx_std_state_t *ls, const char *key, int32_t d, int32_t delta) {
    mx_lua_type_t t = mx_std_getfield(ls, -1, key);
    mx_obj_int_t got = mx_std_tointeger(ls, -1);
    mx_std_pop(ls, 1);
    int64_t res = got.value;
    if (!got.is_int) {
        if (t != MX_LUA_TYPE_NIL) {
            return mx_std_error(ls, "field '%s' is not an integer", key);
        } else if (d < 0) {
            return mx_std_error(ls, "field '%s' missing in date table", key);
        } else {
            return d;
        }
    } else {
        bool in_range;
        if (res >= 0) {
            in_range = res - delta <= INT32_MAX;
        } else {
            in_range = INT32_MIN + delta <= res;
        }
        if (!in_range) {
            return mx_std_error(ls, "field '%s' is out-of-bound", key);
        } else {
            return (int32_t)(res - delta);
        }
    }
}

void mx_std_os_setfield(mx_std_state_t *ls, const char *key, int32_t value, int32_t delta) {
    mx_std_pushinteger(ls, (int64_t)value + delta);
    mx_std_setfield(ls, -2, key);
}

void mx_std_os_setboolfield(mx_std_state_t *ls, const char *key, int32_t value) {
    if (value >= 0) {
        mx_std_pushboolean(ls, value != 0);
        mx_std_setfield(ls, -2, key);
    }
}

void mx_std_os_setallfields(mx_std_state_t *ls, const struct tm *stm) {
    mx_std_os_setfield(ls, "year", stm->tm_year, 1900);
    mx_std_os_setfield(ls, "month", stm->tm_mon, 1);
    mx_std_os_setfield(ls, "day", stm->tm_mday, 0);
    mx_std_os_setfield(ls, "hour", stm->tm_hour, 0);
    mx_std_os_setfield(ls, "min", stm->tm_min, 0);
    mx_std_os_setfield(ls, "sec", stm->tm_sec, 0);
    mx_std_os_setfield(ls, "yday", stm->tm_yday, 1);
    mx_std_os_setfield(ls, "wday", stm->tm_wday, 1);
    mx_std_os_setboolfield(ls, "isdst", stm->tm_isdst);
}

int32_t mx_std_os_time(mx_std_state_t *ls) {
    if (mx_std_isnoneornil(ls, 1)) {
        mx_std_pushinteger(ls, (int64_t)time(NULL));
        return 1;
    }
    mx_std_checktype(ls, 1, MX_LUA_TYPE_TABLE);
    mx_std_settop(ls, 1);
    int32_t year = mx_std_os_getfield(ls, "year", -1, 1900);
    int32_t mon = mx_std_os_getfield(ls, "month", -1, 1);
    int32_t mday = mx_std_os_getfield(ls, "day", -1, 0);
    int32_t hour = mx_std_os_getfield(ls, "hour", 12, 0);
    int32_t min = mx_std_os_getfield(ls, "min", 0, 0);
    int32_t sec = mx_std_os_getfield(ls, "sec", 0, 0);
    struct tm parts = {
        .tm_year = year,
        .tm_mon = mon,
        .tm_mday = mday,
        .tm_hour = hour,
        .tm_min = min,
        .tm_sec = sec,
        .tm_isdst = -1,
    };
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    time_t t = mktime(&parts);
    if (t == (time_t)-1) {
        return mx_std_error(ls, "time result cannot be represented in this installation");
    }
    struct tm *stm = localtime(&t);
    if (stm != NULL) {
        mx_std_os_setallfields(ls, stm);
    }
    mx_std_pushinteger(ls, (int64_t)t);
    return 1;
}

const char *mx_std_os_checkoption(mx_std_state_t *ls, const char *conv, ptrdiff_t convlen, char *buff) {
    ptrdiff_t n = 0;
    if (convlen >= 1 && conv[0] != '\0' && strchr("aAbBcCdDeFgGhHIjmMnprRStTuUVwWxXyYzZ%", conv[0]) != NULL) {
        n = 1;
    } else if (convlen >= 2 && conv[0] == 'E' && conv[1] != '\0' && strchr("cCxXyY", conv[1]) != NULL) {
        n = 2;
    } else if (convlen >= 2 && conv[0] == 'O' && conv[1] != '\0' && strchr("deHImMSuUVwWy", conv[1]) != NULL) {
        n = 2;
    }
    if (n == 0) {
        mx_std_pushfstring(ls, "invalid conversion specifier '%%%s'", conv);
        mx_std_argerror(ls, 1, mx_std_tostring(ls, -1).ptr);
        return conv;
    }
    memcpy(buff, conv, (size_t)n);
    buff[n] = '\0';
    return conv + n;
}

int32_t mx_std_os_date(mx_std_state_t *ls) {
    mx_std_str_t sv = mx_std_optstring(ls, 1, "%c");
    const char *s = sv.ptr;
    size_t slen = sv.len;
    time_t t;
    if (mx_std_isnoneornil(ls, 2)) {
        t = time(NULL);
    } else {
        t = (time_t)mx_std_checkinteger(ls, 2);
    }
    const char *se = s + slen;
    struct tm *stm;
    if (*s == '!') {
        stm = gmtime(&t);
        s += 1;
    } else {
        stm = localtime(&t);
    }
    if (stm == NULL) {
        return mx_std_error(ls, "date result cannot be represented in this installation");
    } else if (strcmp(s, "*t") == 0) {
        mx_std_createtable(ls, 0, 9);
        mx_std_os_setallfields(ls, stm);
        return 1;
    } else {
        char cc[4];
        cc[0] = '%';
        mx_std_buf_t b;
        mx_std_buf_init(ls, &b);
        while (s < se && !ls->vm->std_error.is_set) {
            if (*s != '%') {
                mx_std_buf_addchar(&b, *s);
                s += 1;
            } else {
                s = mx_std_os_checkoption(ls, s + 1, se - s - 1, cc + 1);
                if (!ls->vm->std_error.is_set) {
                    char tmp[256];
                    size_t reslen = strftime(tmp, sizeof(tmp), cc, stm);
                    mx_std_buf_addlstring(&b, tmp, reslen);
                }
            }
        }
        mx_std_buf_push(&b);
        return 1;
    }
}

int32_t mx_std_os_difftime(mx_std_state_t *ls) {
    time_t t1 = (time_t)mx_std_checkinteger(ls, 1);
    time_t t2 = (time_t)mx_std_optinteger(ls, 2, 0);
    mx_std_pushfloat(ls, difftime(t1, t2));
    return 1;
}

int32_t mx_std_os_getenv(mx_std_state_t *ls) {
    const char *value = getenv(mx_std_checkstring(ls, 1).ptr);
    if (value == NULL) {
        mx_std_pushnil(ls);
    } else {
        mx_std_pushstring(ls, value);
    }
    return 1;
}

int32_t mx_std_os_setlocale(mx_std_state_t *ls) {
    const int32_t cats[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME};
    const char *const opts[] = {"all", "collate", "ctype", "monetary", "numeric", "time", NULL};
    const char *l = mx_std_optstring(ls, 1, NULL).ptr;
    int32_t op = mx_std_checkoption(ls, 2, "all", opts);
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    mx_std_pushstring(ls, setlocale(cats[op], l));
    return 1;
}

int32_t mx_std_os_remove(mx_std_state_t *ls) {
    const char *name = mx_std_checkstring(ls, 1).ptr;
    return mx_std_io_fileresult(ls, remove(name) == 0, name);
}

int32_t mx_std_os_rename(mx_std_state_t *ls) {
    const char *from = mx_std_checkstring(ls, 1).ptr;
    const char *to = mx_std_checkstring(ls, 2).ptr;
    return mx_std_io_fileresult(ls, rename(from, to) == 0, NULL);
}

int32_t mx_std_os_tmpname(mx_std_state_t *ls) {
    char buf[] = "/tmp/lua_XXXXXX";
    int32_t fd = mkstemp(buf);
    if (fd == -1) {
        return mx_std_error(ls, "unable to generate a unique filename");
    }
    close(fd);
    mx_std_pushstring(ls, buf);
    return 1;
}

int32_t mx_std_os_execresult(mx_std_state_t *ls, int32_t stat) {
    if (stat != 0 && errno != 0) {
        return mx_std_io_fileresult(ls, false, NULL);
    } else {
        const char *what;
        int32_t code;
        if ((stat & 0x7f) != 0) {
            what = "signal";
            code = stat & 0x7f;
        } else {
            what = "exit";
            code = (stat >> 8) & 0xff;
        }
        if (*what == 'e' && code == 0) {
            mx_std_pushboolean(ls, true);
        } else {
            mx_std_pushnil(ls);
        }
        mx_std_pushstring(ls, what);
        mx_std_pushinteger(ls, code);
        return 3;
    }
}

int32_t mx_std_os_execute(mx_std_state_t *ls) {
    const char *cmd = mx_std_optstring(ls, 1, NULL).ptr;
    errno = 0;
    int32_t stat = system(cmd);
    if (cmd == NULL) {
        mx_std_pushboolean(ls, stat != 0);
        return 1;
    } else {
        return mx_std_os_execresult(ls, stat);
    }
}

int32_t mx_std_os_exit(mx_std_state_t *ls) {
    int64_t status;
    if (!mx_std_isboolean(ls, 1)) {
        status = mx_std_optinteger(ls, 1, EXIT_SUCCESS);
    } else if (mx_std_toboolean(ls, 1)) {
        status = EXIT_SUCCESS;
    } else {
        status = EXIT_FAILURE;
    }
    if (ls->vm->std_error.is_set) {
        return 0;
    }
    fflush(stdout);
    fflush(stderr);
    if (mx_std_toboolean(ls, 2)) {
        mx_t *vm = ls->vm;
        mx_lua_close(vm);
        mx_vm_free(vm);
    }
    exit((int32_t)status);
}

const mx_std_reg_t mx_std_os_regs[] = {
    {"clock", mx_std_os_clock, NULL},
    {"time", mx_std_os_time, NULL},
    {"date", mx_std_os_date, NULL},
    {"difftime", mx_std_os_difftime, NULL},
    {"getenv", mx_std_os_getenv, NULL},
    {"setlocale", mx_std_os_setlocale, NULL},
    {"remove", mx_std_os_remove, NULL},
    {"rename", mx_std_os_rename, NULL},
    {"tmpname", mx_std_os_tmpname, NULL},
    {"execute", mx_std_os_execute, NULL},
    {"exit", mx_std_os_exit, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_os(mx_std_state_t *ls) {
    mx_std_newlib(ls, "os", mx_std_os_regs);
    mx_std_pop(ls, 1);
}
