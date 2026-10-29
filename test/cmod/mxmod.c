
#include "../../src/lua/rt/api/std.h"

int32_t mxmod_add(mx_std_state_t *ls) {
    int64_t a = mx_std_checkinteger(ls, 1);
    int64_t b = mx_std_checkinteger(ls, 2);
    mx_std_pushinteger(ls, a + b);
    return 1;
}

int32_t mxmod_greet(mx_std_state_t *ls) {
    const char *who = mx_std_checkstring(ls, 1).ptr;
    mx_std_pushfstring(ls, "hello, %s", who);
    return 1;
}

int32_t mxmod_counter_step(mx_std_state_t *ls) {
    mx_obj_t cur = mx_std_upval(ls, 0);
    int64_t n = mx_obj_get_int(cur) + 1;
    mx_std_setupval(ls, 0, mx_obj_of_int(n));
    mx_std_pushinteger(ls, n);
    return 1;
}

int32_t mxmod_counter(mx_std_state_t *ls) {
    mx_std_pushinteger(ls, 0);
    mx_std_pushcfn_up(ls, mxmod_counter_step, "step", 1);
    return 1;
}

int32_t luaopen_mxmod(mx_std_state_t *ls) {
    const mx_std_reg_t fns[] = {
        {"add", mxmod_add},
        {"greet", mxmod_greet},
        {"counter", mxmod_counter},
        {NULL, NULL},
    };
    mx_std_newtable(ls);
    mx_std_register(ls, fns);
    return 1;
}
