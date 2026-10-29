local seed, count = tonumber(arg[1]) or 0, tonumber(arg[2]) or 50

local up = 3
local samples = {
    function(a, b) return a + b * up end,
    function(...) local t = {...} return select('#', ...), t[1] end,
    function(n) local s = 0 for i = 1, n do s = s + i end return s end,
    function(t) local out = {} for k, v in pairs(t) do out[#out + 1] = k .. '=' .. tostring(v) end return table.concat(out, ',') end,
    function(x) local function inner(y) return x * y end return inner(2), inner end,
    function(s) return s:upper(), #s, s:sub(2, 3) end,
    function(o) o.n = (o.n or 0) + 1 return o:get() end,
    function(a) if a > 1 then return 'big' elseif a < 0 then return 'neg' else return nil end end,
    function(...t) return t.n, t[2] end,
    function(...) local c <close> = nil goto done ::done:: return select(1, ...) end,
    function(n) local f = function(k) return k end while n > 0 do n = n - f(1) end return n end,
}

math.randomseed(seed)
for _ = 1, count do
    local f = samples[math.random(#samples)]
    local d = string.dump(f, math.random() < 0.5)
    local bytes = {d:byte(1, -1)}
    local kind = math.random(3)
    if kind == 1 then
        for _ = 1, math.random(1, 4) do
            local at = math.random(#bytes)
            bytes[at] = math.random(0, 255)
        end
    elseif kind == 2 then
        for i = math.random(#bytes), #bytes do
            bytes[i] = nil
        end
    else
        table.insert(bytes, math.random(#bytes), math.random(0, 255))
    end
    local parts = {}
    for i = 1, #bytes do
        parts[i] = string.char(bytes[i])
    end
    local g = load(table.concat(parts), 'chunk', 'b')
    if g ~= nil then
        debug.sethook(function() error('step budget') end, '', 1000000)
        pcall(g, 2, 3)
        pcall(g, {n = 1, get = function(self) return self.n end})
        pcall(g, 'abc')
        pcall(g, 4)
        debug.sethook()
    end
end
print('ok')
