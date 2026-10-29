local big = {}
for i = 1, 1000 do
    big[i] = i
end
print(#{table.unpack({}, 1, 300)})
print(#{table.unpack(big)})
local t = {table.unpack(big)}
print(t[1], t[256], t[257], t[1000], t[1001])
local u = {"a", "b", table.unpack(big)}
print(#u, u[1], u[2], u[3], u[258], u[259], u[1002])
print(select("#", table.unpack(big)))
print(select(900, table.unpack(big, 1, 901)))
print(select(-1, table.unpack(big)))
print(math.max(table.unpack(big)), math.min(table.unpack(big)))
local bytes = {}
for i = 1, 600 do
    bytes[i] = 65 + i % 26
end
local s = string.char(table.unpack(bytes))
print(#s, s:sub(1, 5), s:sub(-5))
print(table.pack(table.unpack(big)).n)
local a, b, c = table.unpack(big)
print(a, b, c)
print((table.unpack(big)))
print(pcall(function() return #{table.unpack(big, 1, 257)} end))
print(pcall(function() return #{table.unpack(big, 1, 256)} end))
print(pcall(function() return #{table.unpack(big, 1, 255)} end))
local function count(...)
    return select("#", ...)
end
print(pcall(count, table.unpack(big, 1, 200)))
print(string.format("%d %d", table.unpack(big)))
for round = 1, 20 do
    local r = {table.unpack(big)}
    assert(#r == 1000 and r[777] == 777)
end
print("rounds ok")
print(pcall(table.unpack, {}, 1, 1e7))
