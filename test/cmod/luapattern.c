#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define GRID_MT "luapattern.grid"

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

    lua_newtable(L); /* Result stems table */

    for (lua_Integer i = 1; i <= len; i++) {
        lua_rawgeti(L, 1, i);
        if (lua_type(L, -1) == LUA_TNUMBER) {
            double val = lua_tonumber(L, -1);
            int64_t stem = (int64_t)floor(val / stem_size);
            double leaf = val - ((double)stem * stem_size);

            /* Look up leaf array for this stem */
            lua_pushinteger(L, stem);
            if (lua_rawget(L, -3) != LUA_TTABLE) {
                lua_pop(L, 1); /* pop nil */
                lua_newtable(L);
                lua_pushinteger(L, stem);
                lua_pushvalue(L, -2);
                lua_rawset(L, -5); /* stems[stem] = leaf_array */
            }

            /* Append leaf to leaf array */
            lua_len(L, -1);
            lua_Integer leaf_len = lua_tointeger(L, -1);
            lua_pop(L, 1);

            lua_pushnumber(L, leaf);
            lua_rawseti(L, -2, leaf_len + 1);
            lua_pop(L, 1); /* pop leaf array */
        }
        lua_pop(L, 1); /* pop element */
    }

    return 1;
}

/* O(1) Bitwise Scalar Pattern Memoization */
static int pattern_memo_call(lua_State *L) {
    int nargs = lua_gettop(L);
    if (nargs == 1 && lua_type(L, 1) == LUA_TNUMBER) {
        lua_Number n = lua_tonumber(L, 1);
        uint64_t bits = *(uint64_t *)&n;

        /* Check cache */
        lua_pushvalue(L, lua_upvalueindex(2)); /* cache table */
        lua_pushinteger(L, (lua_Integer)bits);
        if (lua_rawget(L, -2) != LUA_TNIL) {
            return 1; /* Cache hit! */
        }
        lua_pop(L, 1); /* pop nil */

        /* Cache miss: call target fn */
        lua_pushvalue(L, lua_upvalueindex(1));
        lua_pushnumber(L, n);
        lua_call(L, 1, 1);

        /* Store in cache */
        lua_pushvalue(L, lua_upvalueindex(2));
        lua_pushinteger(L, (lua_Integer)bits);
        lua_pushvalue(L, -3); /* result */
        lua_rawset(L, -3);
        lua_pop(L, 1); /* pop cache table */
        return 1;
    }

    /* Fallback: direct call for multi-arg or non-number */
    lua_pushvalue(L, lua_upvalueindex(1));
    for (int i = 1; i <= nargs; i++) {
        lua_pushvalue(L, i);
    }
    lua_call(L, nargs, 1);
    return 1;
}

static int pattern_memo(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);       /* upvalue 1: function */
    lua_newtable(L);           /* upvalue 2: cache table */
    lua_pushinteger(L, 0);     /* upvalue 3: size counter */
    lua_pushcclosure(L, pattern_memo_call, 3);
    return 1;
}

/* O(1) Pattern Prediction Engine (Linear, Geometric, Cycle, Constant) */
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

    /* Read up to the last 8 entries */
    int k = (n > 8) ? 8 : (int)n;
    double vals[8];
    for (int i = 0; i < k; i++) {
        lua_rawgeti(L, 1, n - k + 1 + i);
        vals[i] = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    /* 1. Constant check */
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

    /* 2. Linear / Arithmetic check (1st difference) */
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

    /* 3. Geometric check (ratio) */
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

    /* 4. Repeating Cycle check (cycle length 2, 3, 4) */
    for (int period = 2; period <= k / 2; period++) {
        bool cyclic = true;
        for (int i = 0; i < k - period; i++) {
            if (fabs(vals[k - 1 - i] - vals[k - 1 - i - period]) > 1e-6) {
                cyclic = false;
                break;
            }
        }
        if (cyclic) {
            int next_offset = period - 1;
            lua_pushnumber(L, vals[k - period]);
            lua_pushnumber(L, 0.90);
            lua_pushstring(L, "cycle");
            return 3;
        }
    }

    /* Fallback: Extrapolate with average delta */
    double avg_d = (vals[k - 1] - vals[0]) / (k - 1);
    lua_pushnumber(L, vals[k - 1] + avg_d);
    lua_pushnumber(L, 0.40);
    lua_pushstring(L, "trend");
    return 3;
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
    {NULL, NULL},
};

int luaopen_luapattern(lua_State *L) {
    luaL_checkversion(L);
    luaL_newmetatable(L, GRID_MT);
    luaL_newlib(L, grid_methods);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);
    luaL_newlib(L, pattern_fns);
    return 1;
}
