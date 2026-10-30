-- testes gc.lua "__gc x weak tables": a finalizer set after setmetatable runs,
-- and sees the weak entries reachable only through the dying object cleared.
-- u is read once more after __gc is set: a local that is never read again is
-- not a root here, unlike reference Lua.
local u = setmetatable({}, {__gc = true})
local m = getmetatable(u)
m.x = {[{0}] = 1; [0] = {1}}
setmetatable(m.x, {__mode = "kv"})
local left = "unset"
m.__gc = function (o)
    left = next(getmetatable(o).x)
    m = 10
end
local keep = u
u, m, keep = nil
collectgarbage()
print(m, left)
