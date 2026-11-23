#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GRID_MT "luapattern.grid"
#define CHEB_MT "luapattern.chebyshev"
#define TM_DOZENAL_FULL_CIRCLE 3072

/* Spatial Hash Grid for O(1) Localized Neighbor Queries */
typedef struct {
    int64_t cell_x;
    int64_t cell_y;
    int64_t id;
    double x;
    double y;
} grid_item_t;

typedef struct {
    double cell_size;
    uint32_t cap;
    uint32_t count;
    grid_item_t *items;
} spatial_grid_t;

static inline uint32_t spatial_hash(int64_t cx, int64_t cy, uint32_t mask) {
    uint64_t h = ((uint64_t)cx * UINT64_C(73856093)) ^ ((uint64_t)cy * UINT64_C(19349663));
    h ^= h >> 16;
    h *= UINT64_C(0x9e3779b97f4a7c15);
    return (uint32_t)(h & mask);
}

static int grid_new(lua_State *L) {
    double size = luaL_optnumber(L, 1, 16.0);
    if (size <= 0.0) {
        return luaL_error(L, "cell_size must be positive");
    }
    spatial_grid_t *g = (spatial_grid_t *)lua_newuserdatauv(L, sizeof(spatial_grid_t), 0);
    g->cell_size = size;
    g->cap = 1024;
    g->count = 0;
    g->items = (grid_item_t *)calloc(g->cap, sizeof(grid_item_t));
    for (uint32_t i = 0; i < g->cap; i++) {
        g->items[i].id = -1;
    }
    luaL_getmetatable(L, GRID_MT);
    lua_setmetatable(L, -2);
    return 1;
}

static int grid_gc(lua_State *L) {
    spatial_grid_t *g = (spatial_grid_t *)luaL_checkudata(L, 1, GRID_MT);
    if (g->items != NULL) {
        free(g->items);
        g->items = NULL;
    }
    return 0;
}

static int grid_clear(lua_State *L) {
    spatial_grid_t *g = (spatial_grid_t *)luaL_checkudata(L, 1, GRID_MT);
    for (uint32_t i = 0; i < g->cap; i++) {
        g->items[i].id = -1;
    }
    g->count = 0;
    return 0;
}

static void grid_grow(spatial_grid_t *g) {
    uint32_t old_cap = g->cap;
    grid_item_t *old_items = g->items;
    g->cap = old_cap * 2;
    g->items = (grid_item_t *)calloc(g->cap, sizeof(grid_item_t));
    for (uint32_t i = 0; i < g->cap; i++) {
        g->items[i].id = -1;
    }
    g->count = 0;
    uint32_t mask = g->cap - 1;
    for (uint32_t i = 0; i < old_cap; i++) {
        if (old_items[i].id >= 0) {
            uint32_t slot = spatial_hash(old_items[i].cell_x, old_items[i].cell_y, mask);
            while (g->items[slot].id >= 0) {
                slot = (slot + 1) & mask;
            }
            g->items[slot] = old_items[i];
            g->count++;
        }
    }
    free(old_items);
}

static int grid_insert(lua_State *L) {
    spatial_grid_t *g = (spatial_grid_t *)luaL_checkudata(L, 1, GRID_MT);
    lua_Integer id = luaL_checkinteger(L, 2);
    double x = luaL_checknumber(L, 3);
    double y = luaL_checknumber(L, 4);

    if (g->count * 2 >= g->cap) {
        grid_grow(g);
    }

    int64_t cx = (int64_t)floor(x / g->cell_size);
    int64_t cy = (int64_t)floor(y / g->cell_size);
    uint32_t mask = g->cap - 1;
    uint32_t slot = spatial_hash(cx, cy, mask);

    while (g->items[slot].id >= 0) {
        slot = (slot + 1) & mask;
    }
    g->items[slot].cell_x = cx;
    g->items[slot].cell_y = cy;
    g->items[slot].id = id;
    g->items[slot].x = x;
    g->items[slot].y = y;
    g->count++;
    return 0;
}

static int grid_query(lua_State *L) {
    spatial_grid_t *g = (spatial_grid_t *)luaL_checkudata(L, 1, GRID_MT);
    double x = luaL_checknumber(L, 2);
    double y = luaL_checknumber(L, 3);
    double radius = luaL_optnumber(L, 4, g->cell_size);
    bool exact_distance = lua_toboolean(L, 5);
    double r2 = radius * radius;

    int64_t min_cx = (int64_t)floor((x - radius) / g->cell_size);
    int64_t max_cx = (int64_t)floor((x + radius) / g->cell_size);
    int64_t min_cy = (int64_t)floor((y - radius) / g->cell_size);
    int64_t max_cy = (int64_t)floor((y + radius) / g->cell_size);

    lua_newtable(L);
    int out_idx = 1;
    uint32_t mask = g->cap - 1;

    for (int64_t cx = min_cx; cx <= max_cx; cx++) {
        for (int64_t cy = min_cy; cy <= max_cy; cy++) {
            uint32_t slot = spatial_hash(cx, cy, mask);
            uint32_t probes = 0;
            while (g->items[slot].id >= 0 && probes < g->cap) {
                if (g->items[slot].cell_x == cx && g->items[slot].cell_y == cy) {
                    if (!exact_distance) {
                        lua_pushinteger(L, g->items[slot].id);
                        lua_rawseti(L, -2, out_idx++);
                    } else {
                        double dx = g->items[slot].x - x;
                        double dy = g->items[slot].y - y;
                        if (dx * dx + dy * dy <= r2) {
                            lua_pushinteger(L, g->items[slot].id);
                            lua_rawseti(L, -2, out_idx++);
                        }
                    }
                }
                slot = (slot + 1) & mask;
                probes++;
            }
        }
    }
    return 1;
}

static int grid_count(lua_State *L) {
    spatial_grid_t *g = (spatial_grid_t *)luaL_checkudata(L, 1, GRID_MT);
    lua_pushinteger(L, g->count);
    return 1;
}

/* Fast Discretization / Geometric Quantization */
static int pattern_quantize(lua_State *L) {
    double val = luaL_checknumber(L, 1);
    double step = luaL_optnumber(L, 2, 1.0);
    if (step <= 0.0) {
        return luaL_error(L, "step must be positive");
    }
    lua_pushnumber(L, floor(val / step + 0.5) * step);
    return 1;
}

/* Stem-and-Leaf Decomposition for Geometric Radix Grouping */
static int pattern_stemleaf(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    double stem_size = luaL_optnumber(L, 2, 10.0);
    if (stem_size <= 0.0) {
        return luaL_error(L, "stem_size must be positive");
    }

    lua_len(L, 1);
    lua_Integer len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_newtable(L);

    for (lua_Integer i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, i);
        if (lua_type(L, -1) == LUA_TNUMBER) {
            double val = lua_tonumber(L, -1);
            int64_t stem = (int64_t)floor(val / stem_size);
            double leaf = val - ((double)stem * stem_size);

            lua_pushinteger(L, stem);
            if (lua_rawget(L, -3) != LUA_TTABLE) {
                lua_pop(L, 1);
                lua_newtable(L);
                lua_pushinteger(L, stem);
                lua_pushvalue(L, -2);
                lua_rawset(L, -5);
            }

            lua_len(L, -1);
            lua_Integer leaf_len = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_pushnumber(L, leaf);
            lua_rawseti(L, -2, leaf_len + 1);
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    return 1;
}

/* O(1) Bitwise Scalar Pattern Memoization */
static int pattern_memo_call(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs == 1 && lua_type(L, 1) == LUA_TNUMBER) {
        lua_Number n = lua_tonumber(L, 1);
        uint64_t bits = *(uint64_t *)&n;

        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushinteger(L, (lua_Integer)bits);
        if (lua_rawget(L, -2) != LUA_TNIL) {
            return 1;
        }
        lua_pop(L, 1);

        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushnumber(L, n);
        lua_call(L, 1, 1);

        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushinteger(L, (lua_Integer)bits);
        lua_pushvalue(L, -3);
        lua_rawset(L, -3);
        lua_pop(L, 1);
        return 1;
    }

    lua_pushvalue(L, lua_upvalueindex(1));
    for (int i = 1; i <= nargs; i++) {
        lua_pushvalue(L, i);
    }
    lua_call(L, nargs, 1);
    return 1;
}

static int pattern_memo(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, 0);
    lua_pushcclosure(L, pattern_memo_call, 3);
    return 1;
}

/* O(1) Pattern Prediction Engine */
static int pattern_predict(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_len(L, 1);
    lua_Integer n = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (n < 2) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "none");
        return 3;
    }

    int k = (n > 8) ? 8 : (int)n;
    double vals[8];
    for (int i = 0; i < k; i++) {
        lua_rawgeti(L, 1, n - k + 1 + i);
        vals[i] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    bool constant = true;
    for (int i = 1; i < k; i++) {
        if (fabs(vals[i] - vals[0]) > 1e-9) {
            constant = false;
            break;
        }
    }
    if (constant) {
        lua_pushnumber(L, vals[k - 1]);
        lua_pushnumber(L, 1.0);
        lua_pushstring(L, "constant");
        return 3;
    }

    double d = vals[1] - vals[0];
    bool linear = true;
    for (int i = 2; i < k; i++) {
        if (fabs((vals[i] - vals[i - 1]) - d) > 1e-6) {
            linear = false;
            break;
        }
    }
    if (linear) {
        lua_pushnumber(L, vals[k - 1] + d);
        lua_pushnumber(L, 1.0);
        lua_pushstring(L, "linear");
        return 3;
    }

    if (fabs(vals[0]) > 1e-9) {
        double r = vals[1] / vals[0];
        bool geometric = true;
        for (int i = 2; i < k; i++) {
            if (fabs(vals[i - 1]) < 1e-9 || fabs((vals[i] / vals[i - 1]) - r) > 1e-6) {
                geometric = false;
                break;
            }
        }
        if (geometric) {
            lua_pushnumber(L, vals[k - 1] * r);
            lua_pushnumber(L, 0.95);
            lua_pushstring(L, "geometric");
            return 3;
        }
    }

    for (int period = 2; period <= k / 2; period++) {
        bool cyclic = true;
        for (int i = 0; i < k - period; i++) {
            if (fabs(vals[k - 1 - i] - vals[k - 1 - i - period]) > 1e-6) {
                cyclic = false;
                break;
            }
        }
        if (cyclic) {
            lua_pushnumber(L, vals[k - period]);
            lua_pushnumber(L, 0.90);
            lua_pushstring(L, "cycle");
            return 3;
        }
    }

    double avg_d = (vals[k - 1] - vals[0]) / (k - 1);
    lua_pushnumber(L, vals[k - 1] + avg_d);
    lua_pushnumber(L, 0.40);
    lua_pushstring(L, "trend");
    return 3;
}

/* ========================================================================= */
/* Spectral Fourier & Harmonic Extensions                                    */
/* ========================================================================= */

/* Fast Walsh-Hadamard Transform (FWHT) for Boolean logic and truth tables */
static int pattern_wht(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_len(L, 1);
    lua_Integer n = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (n <= 0 || (n & (n - 1)) != 0) {
        return luaL_error(L, "table size must be a power of 2");
    }

    int32_t *buf = (int32_t *)malloc(n * sizeof(int32_t));
    for (lua_Integer i = 1; i <= n; i++) {
        lua_rawgeti(L, 1, i);
        buf[i - 1] = (int32_t)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }

    /* In-place butterfly network */
    for (size_t len = 1; len < (size_t)n; len <<= 1) {
        for (size_t i = 0; i < (size_t)n; i += 2 * len) {
            for (size_t j = 0; j < len; j++) {
                int32_t u = buf[i + j];
                int32_t v = buf[i + len + j];
                buf[i + j] = u + v;
                buf[i + len + j] = u - v;
            }
        }
    }

    lua_newtable(L);
    for (size_t i = 0; i < (size_t)n; i++) {
        lua_pushinteger(L, buf[i]);
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }

    free(buf);
    return 1;
}

/* Branchless Fourier evaluation: sum coeff * (-1)^{<mask, x>} */
static int pattern_wht_eval(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    uint32_t input_mask = (uint32_t)luaL_checkinteger(L, 2);

    lua_len(L, 1);
    lua_Integer count = lua_tointeger(L, -1);
    lua_pop(L, 1);

    int32_t sum = 0;
    for (lua_Integer i = 1; i <= count; i++) {
        lua_rawgeti(L, 1, i);
        if (lua_istable(L, -1)) {
            lua_rawgeti(L, -1, 1);
            uint32_t term_mask = (uint32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_rawgeti(L, -1, 2);
            int32_t coeff = (int32_t)lua_tointeger(L, -1);
            lua_pop(L, 1);

            uint32_t masked = term_mask & input_mask;
#if defined(__GNUC__) || defined(__clang__)
            int parity = __builtin_parity(masked);
#else
            masked ^= masked >> 16;
            masked ^= masked >> 8;
            masked ^= masked >> 4;
            masked &= 0xf;
            int parity = (0x6996 >> masked) & 1;
#endif
            sum += (parity ? -coeff : coeff);
        }
        lua_pop(L, 1);
    }

    lua_pushinteger(L, sum);
    return 1;
}

/* Dozenal (Base-12) Phase Conversion: 2*pi = 3072 units */
static int pattern_dozenal(lua_State *L) {
    double rad = luaL_checknumber(L, 1);
    double two_pi = 2.0 * M_PI;
    double norm = fmod(rad, two_pi);
    if (norm < 0.0) norm += two_pi;
    uint32_t units = (uint32_t)floor((norm / two_pi) * (double)TM_DOZENAL_FULL_CIRCLE + 0.5);
    lua_pushinteger(L, units % TM_DOZENAL_FULL_CIRCLE);
    return 1;
}

/* Chebyshev Function Compression Evaluator Closure */
typedef struct {
    double a;
    double b;
    size_t degree;
    double coeffs[16];
} cheb_closure_t;

static int cheb_eval_call(lua_State *L) {
    cheb_closure_t *cc = (cheb_closure_t *)luaL_checkudata(L, lua_upvalueindex(1), CHEB_MT);
    double x = luaL_checknumber(L, 1);

    double y = (2.0 * x - (cc->b + cc->a)) / (cc->b - cc->a);
    double y2 = 2.0 * y;
    double d1 = 0.0;
    double d2 = 0.0;

    for (size_t j = cc->degree; j >= 1; j--) {
        double d0 = y2 * d1 - d2 + cc->coeffs[j];
        d2 = d1;
        d1 = d0;
    }

    double res = y * d1 - d2 + 0.5 * cc->coeffs[0];
    lua_pushnumber(L, res);
    return 1;
}

static int pattern_chebyshev(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    double a = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    size_t degree = (size_t)luaL_optinteger(L, 4, 6);
    if (degree > 15) degree = 15;
    if (b <= a) return luaL_error(L, "b must be greater than a");

    size_t num_nodes = degree * 2 + 16;
    double *f_vals = (double *)malloc(num_nodes * sizeof(double));

    for (size_t k = 0; k < num_nodes; k++) {
        double theta = M_PI * ((double)k + 0.5) / (double)num_nodes;
        double z = cos(theta);
        double x = 0.5 * (b - a) * z + 0.5 * (b + a);

        lua_pushvalue(L, 1);
        lua_pushnumber(L, x);
        lua_call(L, 1, 1);
        f_vals[k] = lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    cheb_closure_t *cc = (cheb_closure_t *)lua_newuserdatauv(L, sizeof(cheb_closure_t), 0);
    cc->a = a;
    cc->b = b;
    cc->degree = degree;

    for (size_t j = 0; j <= degree; j++) {
        double sum = 0.0;
        for (size_t k = 0; k < num_nodes; k++) {
            double theta = M_PI * ((double)k + 0.5) / (double)num_nodes;
            sum += f_vals[k] * cos((double)j * theta);
        }
        cc->coeffs[j] = (2.0 / (double)num_nodes) * sum;
    }
    free(f_vals);

    luaL_getmetatable(L, CHEB_MT);
    lua_setmetatable(L, -2);

    /* Return closure with cheb_closure_t as upvalue */
    lua_pushcclosure(L, cheb_eval_call, 1);
    return 1;
}

/* Discrete Fourier Phase Detector */
static int pattern_phase(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_len(L, 1);
    lua_Integer len = lua_tointeger(L, -1);
    lua_pop(L, 1);

    if (len < 8) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    double *sig = (double *)malloc(len * sizeof(double));
    double mean = 0.0;
    for (lua_Integer i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, i);
        sig[i - 1] = luaL_checknumber(L, -1);
        mean += sig[i - 1];
        lua_pop(L, 1);
    }
    mean /= (double)len;

    size_t num_bins = (size_t)len / 2;
    double max_power = 0.0;
    size_t best_k = 0;
    double total_power = 0.0;

    for (size_t k = 1; k < num_bins; k++) {
        double real = 0.0;
        double imag = 0.0;
        double omega = 2.0 * M_PI * (double)k / (double)len;

        for (size_t n = 0; n < (size_t)len; n++) {
            double val = sig[n] - mean;
            real += val * cos(omega * (double)n);
            imag -= val * sin(omega * (double)n);
        }

        double power = real * real + imag * imag;
        total_power += power;
        if (power > max_power) {
            max_power = power;
            best_k = k;
        }
    }
    free(sig);

    if (total_power < 1e-9) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    lua_pushnumber(L, (double)best_k / (double)len);
    lua_pushnumber(L, max_power / total_power);
    return 2;
}

static const luaL_Reg grid_methods[] = {
    {"insert", grid_insert},
    {"query", grid_query},
    {"clear", grid_clear},
    {"count", grid_count},
    {"__gc", grid_gc},
    {NULL, NULL},
};

static const luaL_Reg pattern_fns[] = {
    {"grid", grid_new},
    {"quantize", pattern_quantize},
    {"stemleaf", pattern_stemleaf},
    {"memo", pattern_memo},
    {"predict", pattern_predict},
    {"wht", pattern_wht},
    {"wht_eval", pattern_wht_eval},
    {"dozenal", pattern_dozenal},
    {"chebyshev", pattern_chebyshev},
    {"phase", pattern_phase},
    {NULL, NULL},
};

int luaopen_luapattern(lua_State *L) {
    luaL_checkversion(L);
    luaL_newmetatable(L, GRID_MT);
    luaL_newlib(L, grid_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, CHEB_MT);
    lua_pop(L, 1);

    luaL_newlib(L, pattern_fns);
    return 1;
}
