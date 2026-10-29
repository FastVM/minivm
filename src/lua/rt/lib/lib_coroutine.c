
#include "../api/std.h"
#include "../coro.h"

const mx_std_reg_t mx_std_coroutine_regs[] = {
    {"create", mx_coro_create, NULL},
    {"resume", mx_coro_resume, NULL},
    {"yield", mx_coro_yield, NULL},
    {"status", mx_coro_status, NULL},
    {"running", mx_coro_running, NULL},
    {"isyieldable", mx_coro_isyieldable, NULL},
    {"close", mx_coro_close, NULL},
    {"wrap", mx_coro_wrap, NULL},
    {NULL, NULL, NULL},
};

void mx_std_open_coroutine(mx_std_state_t *ls) {
    mx_std_newlib(ls, "coroutine", mx_std_coroutine_regs);
    mx_std_pop(ls, 1);
}
