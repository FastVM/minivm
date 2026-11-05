local function show(...)
    local parts = {}
    for i = 1, select("#", ...) do
        parts[i] = tostring(select(i, ...))
    end
    print(table.concat(parts, " "))
end

local function rep(f, ...)
    local a, b, c, d
    for _ = 1, 12 do
        a, b, c, d = f(...)
    end
    show(a, b, c, d)
end

local ints = {0, 1, -1, 2, -2, 3, -3, 7, -7, 8, -8, 16, 63, 64, 65, -63, -64, -65, 1 << 31, -(1 << 31), (1 << 31) - 1, 1 << 32, math.maxinteger, math.mininteger}

local function idiv(a, b) return a // b end
local function mod(a, b) return a % b end
for _, a in ipairs(ints) do
    for _, b in ipairs(ints) do
        if b ~= 0 then
            rep(idiv, a, b)
            rep(mod, a, b)
        else
            show(pcall(idiv, a, b))
            show(pcall(mod, a, b))
        end
    end
end

local function const_div(a)
    return a // 1, a // 2, a // 8, a // 3, a // -1, a // -2, a // -3, a // 1024
end
local function const_mod(a)
    return a % 1, a % 2, a % 8, a % 3, a % -1, a % -2, a % -3, a % 1024
end
for _, a in ipairs(ints) do
    for _ = 1, 12 do
        const_div(a)
        const_mod(a)
    end
    show(const_div(a))
    show(const_mod(a))
end

local function lit_div(b) return 100 // b, 100 % b, -100 // b, -100 % b end
for _, b in ipairs({1, -1, 3, -3, 7, 1000, -1000}) do
    rep(lit_div, b)
end
show(pcall(function() local x = 5; for _ = 1, 12 do x = x // 0 end return x end))
show(pcall(function() local x = 5; for _ = 1, 12 do x = x % 0 end return x end))
show(pcall(function() local z = 0; local x = 5; for _ = 1, 12 do x = 9 // z end return x end))

local function bits(a, b)
    return a & b, a | b, a ~ b, ~a, a << b, a >> b
end
local function const_bits(a)
    return a & 255, a | 255, a ~ 255, a << 0, a << 1, a << 63, a << 64, a << -1, a << -64, a >> 0, a >> 1, a >> 63, a >> 64, a >> -1, a >> -64
end
local function big_bits(a)
    return a & 0x123456789ab, a | 0x123456789ab, a ~ 0x123456789ab, a + 0x123456789ab, a - 0x123456789ab, a * 0x123456789ab
end
for _, a in ipairs(ints) do
    for _, b in ipairs({0, 1, 3, 63, 64, 65, -1, -63, -64, -65}) do
        rep(bits, a, b)
    end
    for _ = 1, 12 do
        const_bits(a)
        big_bits(a)
    end
    show(const_bits(a))
    show(big_bits(a))
end
local function lit_shift(b) return 1 << b, 1 >> b, -1 << b, -1 >> b end
for _, b in ipairs({0, 1, 63, 64, -1, -64}) do
    rep(lit_shift, b)
end

local function arith(a, b)
    return a + b, a - b, a * b, -a, b - a
end
local function lit_arith(a)
    return a + 1, 1 + a, a - 1, 1 - a, a * 3, 3 * a, -a, a + 0x7fffffff, a + 0x80000000
end
for _, a in ipairs(ints) do
    for _, b in ipairs({0, 1, -1, 5, math.maxinteger, math.mininteger}) do
        rep(arith, a, b)
    end
    rep(lit_arith, a)
end

local floats = {0.0, -0.0, 1.0, -1.0, 0.5, -0.5, 1.5, 2.5, 3.0, 1e15, 1e300, -1e300, 2^53, -2^53, 2^63, -2^63, math.huge, -math.huge, 0/0}

local function farith(a, b)
    return a + b, a - b, a * b, a / b, a // b, a % b, a ^ b, -a
end
local function flit(a)
    return a + 1.5, 1.5 + a, a - 1.5, 1.5 - a, a * 1.5, 1.5 * a, a / 1.5, 1.5 / a, a // 1.5, 1.5 // a, a % 1.5, 1.5 % a, a ^ 1.5, 1.5 ^ a
end
for _, a in ipairs(floats) do
    for _, b in ipairs(floats) do
        rep(farith, a, b)
    end
    for _ = 1, 12 do
        flit(a)
    end
    show(flit(a))
end

local function mixed(a, b)
    return a + b, a - b, a * b, a / b, a ^ b
end
for _, a in ipairs({0, 1, -3, 1 << 53, (1 << 53) + 1}) do
    for _, b in ipairs({0.0, 1.5, -2.5, 2^53}) do
        rep(mixed, a, b)
        rep(mixed, b, a)
    end
end
local function intdiv(a, b) return a / b, a ^ b end
for _, a in ipairs(ints) do
    rep(intdiv, a, 3)
    rep(intdiv, 3, a)
end

local function cmp(a, b)
    return a < b, a <= b, a > b, a >= b, a == b, a ~= b
end
local function branchy(a, b)
    local n = 0
    if a < b then n = n + 1 end
    if a <= b then n = n + 10 end
    if a > b then n = n + 100 end
    if a >= b then n = n + 1000 end
    if a == b then n = n + 10000 end
    if a ~= b then n = n + 100000 end
    return n
end
local cmp_vals = {0, 1, -1, 3, math.maxinteger, math.mininteger, 0.0, -0.0, 1.0, 1.5, -1.5, 2^53, 2^63, -2^63, math.huge, -math.huge, 0/0}
for _, a in ipairs(cmp_vals) do
    for _, b in ipairs(cmp_vals) do
        rep(cmp, a, b)
        rep(branchy, a, b)
    end
end

local function cmp_lits(a)
    local n = 0
    if a < 5 then n = n + 1 end
    if a <= 5 then n = n + 2 end
    if 5 < a then n = n + 4 end
    if 5 <= a then n = n + 8 end
    if a == 5 then n = n + 16 end
    if 5 == a then n = n + 32 end
    if a < 2.5 then n = n + 64 end
    if a <= 2.5 then n = n + 128 end
    if 2.5 < a then n = n + 256 end
    if 2.5 <= a then n = n + 512 end
    if a == 2.5 then n = n + 1024 end
    if a < 0x100000000 then n = n + 2048 end
    if 0x100000000 < a then n = n + 4096 end
    return n
end
for _, a in ipairs(cmp_vals) do
    rep(cmp_lits, a)
end

local function strcmp(a, b)
    return a < b, a <= b, a == b, a ~= b, a > b, a >= b
end
for _, a in ipairs({"", "a", "b", "ab", "a\0b", "a\0c", string.rep("x", 40), string.rep("x", 40) .. "y"}) do
    for _, b in ipairs({"", "a", "ab", "a\0c", string.rep("x", 40)}) do
        rep(strcmp, a, b)
    end
end

local function many()
    local a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r = 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18
    local s = 0
    for t = 1, 30 do
        a = a + b; b = b + c; c = c + d; d = d + e; e = e + f; f = f + g; g = g + h; h = h + i
        i = i + j; j = j + k; k = k + l; l = l + m; m = m + n; n = n + o; o = o + p; p = p + q
        q = q + r; r = r + t
        s = s + a ~ b ~ c ~ d ~ e ~ f ~ g ~ h ~ i ~ j ~ k ~ l ~ m ~ n ~ o ~ p ~ q ~ r
    end
    return s, a, r
end
rep(many)

local function manyf()
    local a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r = 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 10.5, 11.5, 12.5, 13.5, 14.5, 15.5, 16.5, 17.5, 18.5
    local s = 0.0
    for t = 1, 30 do
        a = a + b * 0.25; b = b - c * 0.25; c = c + d * 0.25; d = d - e * 0.25
        e = e + f * 0.25; f = f - g * 0.25; g = g + h * 0.25; h = h - i * 0.25
        i = i + j * 0.25; j = j - k * 0.25; k = k + l * 0.25; l = l - m * 0.25
        m = m + n * 0.25; n = n - o * 0.25; o = o + p * 0.25; p = p - q * 0.25
        q = q + r * 0.25; r = r - t * 0.25
        s = s + a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p + q + r
    end
    return s, a, r
end
rep(manyf)

local function retype(x)
    local v = x
    for i = 1, 40 do
        if i == 10 then v = v + 0.5 end
        if i == 20 then v = tostring(v) end
        if i == 30 then v = #v end
        if i == 35 then v = nil; v = 1 end
    end
    return v
end
rep(retype, 1)
rep(retype, 1.5)

local function lens(s, t)
    return #s, #t, #"abc", #s + #t
end
rep(lens, "hello", {1, 2, 3})
rep(lens, "", {})
show(pcall(function() local v = 5; for _ = 1, 12 do v = #v end end))
show(pcall(function() local v = {}; for _ = 1, 12 do v = v + 1 end end))
