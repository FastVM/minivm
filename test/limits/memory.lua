local function try(f, ...)
    local ok, v = pcall(f, ...)
    if type(v) == 'string' and #v > 100 then
        v = #v
    end
    print(ok, v)
end
try(table.create, 1 << 30)
try(string.rep, 'x', (1 << 31) - 1)
try(string.rep, 'xy', 1 << 29, ',')
try(string.upper, string.rep('x', 1 << 20))
local parts = {}
for i = 1, 3 do
    parts[i] = string.rep('x', 1 << 30)
end
try(table.concat, parts, ',')
parts = nil
for _ = 1, 100 do
    local s = string.rep('x', 1 << 26)
end
print('alive')
