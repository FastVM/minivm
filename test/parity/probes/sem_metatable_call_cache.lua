local function add(self, x) return x + self.k end
local function mul(self, x) return x * self.k end
local mt = {__call = add}
local obj = setmetatable({k = 3}, mt)

local function site(o, x)
    local r1, r2 = o(x)
    return r1, r2
end

local out = {}
for i = 1, 4 do
    out[#out + 1] = site(obj, i)
end
mt.__call = mul
out[#out + 1] = site(obj, 10)
for i = 1, 40 do
    mt['pad' .. i] = i
end
out[#out + 1] = site(obj, 11)
setmetatable(obj, {__call = add})
out[#out + 1] = site(obj, 12)
print(table.concat(out, ' '))

local a = setmetatable({k = 1}, {__call = add})
local b = setmetatable({k = 2}, {__call = mul})
local c = setmetatable({k = 5}, {__call = function(self, ...) return select('#', ...), ... end})
local d = setmetatable({}, {__call = function() return 'none' end})
local e = setmetatable({}, {__call = c})
local acc = {}
for i = 1, 3 do
    for _, o in ipairs({a, b, c, d, e}) do
        acc[#acc + 1] = tostring((site(o, i)))
    end
end
print(table.concat(acc, ' '))
print(c(1, nil, 3))

local f = setmetatable({}, {__call = function(self, x)
    local info = debug.getinfo(1, 't')
    return info.extraargs, x
end})
print(site(f, 'x'))

getmetatable(obj).__call = nil
print(pcall(site, obj, 1))
getmetatable(obj).__call = 42
print(pcall(site, obj, 1))

local g = setmetatable({}, {__call = function(self, x) return x.missing.field end})
print(pcall(site, g, {}))
