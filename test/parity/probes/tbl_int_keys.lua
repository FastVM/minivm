local t = {10, 20, 30}
print(t[1], t[3], t[4], t[0], t[-1])
t[4] = 40
t[2] = nil
t[6] = 60
print(t[1], t[2], t[4], t[5], t[6], #t >= 1)

local log = {}
local proxy = setmetatable({}, {
    __index = function(_, k) return 'idx' .. k end,
    __newindex = function(raw, k, v) log[#log + 1] = k .. '=' .. tostring(v) rawset(raw, k, v) end,
})
print(proxy[1], proxy[2])
proxy[1] = 'a'
proxy[3] = 'c'
proxy[1] = 'b'
print(proxy[1], proxy[2], proxy[3], table.concat(log, ' '))

local ro = setmetatable({}, {__index = {7, 8, 9}})
print(ro[1], ro[3], ro[4])

local function make(i)
    return {i, i + 1, [5] = 'five', x = 'ex', i + 2}
end
local m = make(1)
print(m[1], m[2], m[3], m[4], m[5], m.x, #m)
local sparse = {[3] = 'c', [1] = 'a', [2] = 'b'}
print(sparse[1], sparse[2], sparse[3], #sparse)
local holes = {1, nil, 3}
print(holes[1], holes[2], holes[3], select('#', table.unpack(holes, 1, 3)))

local arr = {}
for i = 1, 10 do
    arr[i] = i * i
end
arr[11] = 'tail'
print(arr[1], arr[10], arr[11], #arr)

print(math.sqrt(16), math.sqrt(2) > 1.41, math.floor(3.7), math.floor(-3.5), math.floor(7), math.floor(2 ^ 70) == 2 ^ 70)
print(math.ceil(3.2), math.ceil(-3.2), math.abs(-5), math.abs(-2.5), math.abs(math.mininteger) == math.mininteger)
print(math.max(1, 2.5), math.min(1, 2.5), math.max(3, 3.0), math.min(-0.0, 0), math.max(1, 2, 3))
print(string.len('hello'), string.len(''), pcall(math.sqrt, 'x'), pcall(string.len))
print(math.type(math.floor(3.0)), math.type(math.sqrt(4)), math.type(math.max(1, 2)))

local hits = 0
debug.sethook(function() hits = hits + 1 end, '', 1000)
local cells = {}
for i = 1, 200 do
    cells[i] = {i}
end
local sum = 0
for _ = 1, 200 do
    for i = 1, 200 do
        sum = sum + cells[i][1]
    end
end
debug.sethook()
print(sum, hits > 0)
