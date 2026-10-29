
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define JSON_MAX_DEPTH 200

typedef struct {
    const char *s;
    size_t len;
    size_t pos;
    int depth;
    lua_State *L;
} json_read;

typedef struct {
    lua_State *L;
    char *data;
    size_t len;
    size_t cap;
} json_out;

static int json_out_gc(lua_State *L) {
    json_out *out = lua_touserdata(L, 1);
    free(out->data);
    out->data = NULL;
    return 0;
}

static void json_addlstring(json_out *B, const char *s, size_t len) {
    if (B->cap - B->len < len) {
        size_t cap = B->cap * 2 + len + 64;
        char *data = realloc(B->data, cap);
        if (data == NULL) {
            luaL_error(B->L, "not enough memory");
        }
        B->data = data;
        B->cap = cap;
    }
    memcpy(B->data + B->len, s, len);
    B->len += len;
}

static void json_addstring(json_out *B, const char *s) {
    json_addlstring(B, s, strlen(s));
}

static void json_addchar(json_out *B, char c) {
    json_addlstring(B, &c, 1);
}

static void json_encode_value(lua_State *L, json_out *B, int idx, int depth);

static void json_encode_string(json_out *B, const char *s, size_t len) {
    json_addchar(B, '"');
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': json_addstring(B, "\\\""); break;
            case '\\': json_addstring(B, "\\\\"); break;
            case '\n': json_addstring(B, "\\n"); break;
            case '\r': json_addstring(B, "\\r"); break;
            case '\t': json_addstring(B, "\\t"); break;
            default: {
                if (c < 0x20) {
                    char esc[7];
                    snprintf(esc, sizeof(esc), "\\u%04x", c);
                    json_addstring(B, esc);
                } else {
                    json_addchar(B, (char)c);
                }
                break;
            }
        }
    }
    json_addchar(B, '"');
}

static int json_is_array(lua_State *L, int idx, size_t *out_n) {
    size_t n = lua_rawlen(L, idx);
    size_t count = 0;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        count += 1;
        lua_pop(L, 1);
    }
    *out_n = n;
    return count == n;
}

static void json_encode_table(lua_State *L, json_out *B, int idx, int depth) {
    size_t n = 0;
    if (json_is_array(L, idx, &n)) {
        json_addchar(B, '[');
        for (size_t i = 1; i <= n; i++) {
            if (i > 1) {
                json_addchar(B, ',');
            }
            lua_rawgeti(L, idx, (lua_Integer)i);
            json_encode_value(L, B, lua_gettop(L), depth + 1);
            lua_pop(L, 1);
        }
        json_addchar(B, ']');
        return;
    }
    json_addchar(B, '{');
    int first = 1;
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        if (!first) {
            json_addchar(B, ',');
        }
        first = 0;
        int kt = lua_type(L, -2);
        if (kt == LUA_TSTRING) {
            size_t klen;
            const char *k = lua_tolstring(L, -2, &klen);
            json_encode_string(B, k, klen);
        } else if (kt == LUA_TNUMBER) {
            lua_pushvalue(L, -2);
            size_t klen;
            const char *k = lua_tolstring(L, -1, &klen);
            json_encode_string(B, k, klen);
            lua_pop(L, 1);
        } else {
            luaL_error(L, "cannot encode table key of type %s", lua_typename(L, kt));
        }
        json_addchar(B, ':');
        json_encode_value(L, B, lua_gettop(L), depth + 1);
        lua_pop(L, 1);
    }
    json_addchar(B, '}');
}

static void json_encode_value(lua_State *L, json_out *B, int idx, int depth) {
    if (depth > JSON_MAX_DEPTH) {
        luaL_error(L, "nesting too deep");
    }
    if (!lua_checkstack(L, 8)) {
        luaL_error(L, "stack overflow while encoding");
    }
    switch (lua_type(L, idx)) {
        case LUA_TNIL: {
            json_addstring(B, "null");
            break;
        }
        case LUA_TBOOLEAN: {
            json_addstring(B, lua_toboolean(L, idx) ? "true" : "false");
            break;
        }
        case LUA_TNUMBER: {
            char buf[48];
            if (lua_isinteger(L, idx)) {
                snprintf(buf, sizeof(buf), "%lld", (long long)lua_tointeger(L, idx));
            } else {
                snprintf(buf, sizeof(buf), "%.14g", (double)lua_tonumber(L, idx));
            }
            json_addstring(B, buf);
            break;
        }
        case LUA_TSTRING: {
            size_t len;
            const char *s = lua_tolstring(L, idx, &len);
            json_encode_string(B, s, len);
            break;
        }
        case LUA_TTABLE: {
            json_encode_table(L, B, idx, depth);
            break;
        }
        default: {
            luaL_error(L, "cannot encode a %s value", lua_typename(L, lua_type(L, idx)));
            break;
        }
    }
}

static int json_encode(lua_State *L) {
    luaL_checkany(L, 1);
    lua_settop(L, 1);
    json_out *B = lua_newuserdatauv(L, sizeof(json_out), 0);
    B->L = L;
    B->data = NULL;
    B->len = 0;
    B->cap = 0;
    if (luaL_newmetatable(L, "luajson.out")) {
        lua_pushcfunction(L, json_out_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);
    json_encode_value(L, B, 1, 0);
    lua_pushlstring(L, B->data, B->len);
    return 1;
}

static void json_skip_ws(json_read *R) {
    while (R->pos < R->len) {
        char c = R->s[R->pos];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        R->pos += 1;
    }
}

static void json_decode_value(json_read *R);

static void json_decode_string(json_read *R) {
    luaL_Buffer B;
    luaL_buffinit(R->L, &B);
    R->pos += 1;
    while (R->pos < R->len && R->s[R->pos] != '"') {
        char c = R->s[R->pos];
        if (c == '\\') {
            R->pos += 1;
            if (R->pos >= R->len) {
                break;
            }
            char e = R->s[R->pos];
            if (e == 'n') {
                luaL_addchar(&B, '\n');
            } else if (e == 't') {
                luaL_addchar(&B, '\t');
            } else if (e == 'r') {
                luaL_addchar(&B, '\r');
            } else if (e == 'u') {
                char hex[5] = {0};
                for (int i = 0; i < 4 && R->pos + 1 + i < R->len; i++) {
                    hex[i] = R->s[R->pos + 1 + i];
                }
                long cp = strtol(hex, NULL, 16);
                R->pos += 4;
                if (cp < 0x80) {
                    luaL_addchar(&B, (char)cp);
                } else {
                    luaL_addchar(&B, (char)(0xC0 | (cp >> 6)));
                    luaL_addchar(&B, (char)(0x80 | (cp & 0x3F)));
                }
            } else {
                luaL_addchar(&B, e);
            }
            R->pos += 1;
            continue;
        }
        luaL_addchar(&B, c);
        R->pos += 1;
    }
    if (R->pos >= R->len) {
        luaL_error(R->L, "unterminated string at %d", (int)R->pos);
    }
    R->pos += 1;
    luaL_pushresult(&B);
}

static void json_decode_array(json_read *R) {
    lua_State *L = R->L;
    lua_newtable(L);
    R->pos += 1;
    lua_Integer i = 1;
    json_skip_ws(R);
    if (R->pos < R->len && R->s[R->pos] == ']') {
        R->pos += 1;
        return;
    }
    while (R->pos < R->len) {
        json_decode_value(R);
        lua_rawseti(L, -2, i);
        i += 1;
        json_skip_ws(R);
        if (R->pos < R->len && R->s[R->pos] == ',') {
            R->pos += 1;
            json_skip_ws(R);
            continue;
        }
        break;
    }
    if (R->pos >= R->len || R->s[R->pos] != ']') {
        luaL_error(L, "expected ']' at %d", (int)R->pos);
    }
    R->pos += 1;
}

static void json_decode_object(json_read *R) {
    lua_State *L = R->L;
    lua_newtable(L);
    R->pos += 1;
    json_skip_ws(R);
    if (R->pos < R->len && R->s[R->pos] == '}') {
        R->pos += 1;
        return;
    }
    while (R->pos < R->len) {
        json_skip_ws(R);
        if (R->s[R->pos] != '"') {
            luaL_error(L, "expected key string at %d", (int)R->pos);
        }
        json_decode_string(R);
        json_skip_ws(R);
        if (R->pos >= R->len || R->s[R->pos] != ':') {
            luaL_error(L, "expected ':' at %d", (int)R->pos);
        }
        R->pos += 1;
        json_skip_ws(R);
        json_decode_value(R);
        lua_rawset(L, -3);
        json_skip_ws(R);
        if (R->pos < R->len && R->s[R->pos] == ',') {
            R->pos += 1;
            continue;
        }
        break;
    }
    if (R->pos >= R->len || R->s[R->pos] != '}') {
        luaL_error(L, "expected '}' at %d", (int)R->pos);
    }
    R->pos += 1;
}

static void json_decode_value(json_read *R) {
    lua_State *L = R->L;
    R->depth += 1;
    if (R->depth > JSON_MAX_DEPTH) {
        luaL_error(L, "nesting too deep");
    }
    if (!lua_checkstack(L, 8)) {
        luaL_error(L, "stack overflow while decoding");
    }
    json_skip_ws(R);
    if (R->pos >= R->len) {
        luaL_error(L, "unexpected end of input");
    }
    char c = R->s[R->pos];
    if (c == '{') {
        json_decode_object(R);
    } else if (c == '[') {
        json_decode_array(R);
    } else if (c == '"') {
        json_decode_string(R);
    } else if (strncmp(R->s + R->pos, "true", 4) == 0) {
        lua_pushboolean(L, 1);
        R->pos += 4;
    } else if (strncmp(R->s + R->pos, "false", 5) == 0) {
        lua_pushboolean(L, 0);
        R->pos += 5;
    } else if (strncmp(R->s + R->pos, "null", 4) == 0) {
        lua_pushnil(L);
        R->pos += 4;
    } else {
        char *end = NULL;
        double d = strtod(R->s + R->pos, &end);
        if (end == R->s + R->pos) {
            luaL_error(L, "unexpected character '%c' at %d", c, (int)R->pos);
        }
        size_t used = (size_t)(end - (R->s + R->pos));
        int isint = 1;
        for (size_t i = 0; i < used; i++) {
            char ch = R->s[R->pos + i];
            if (ch == '.' || ch == 'e' || ch == 'E') {
                isint = 0;
            }
        }
        R->pos += used;
        if (isint && d >= -9.2e18 && d <= 9.2e18) {
            lua_pushinteger(L, (lua_Integer)d);
        } else {
            lua_pushnumber(L, d);
        }
    }
    R->depth -= 1;
}

static int json_decode(lua_State *L) {
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    json_read R;
    R.s = s;
    R.len = len;
    R.pos = 0;
    R.depth = 0;
    R.L = L;
    json_decode_value(&R);
    json_skip_ws(&R);
    if (R.pos != R.len) {
        return luaL_error(L, "trailing garbage at %d", (int)R.pos);
    }
    return 1;
}

static const luaL_Reg json_fns[] = {
    {"encode", json_encode},
    {"decode", json_decode},
    {NULL, NULL},
};

int luaopen_luajson(lua_State *L) {
    luaL_checkversion(L);
    luaL_newlib(L, json_fns);
    lua_pushstring(L, "luajson 1.0");
    lua_setfield(L, -2, "_VERSION");
    return 1;
}
