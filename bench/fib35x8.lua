local ffi = require('ffi')

local function fib(n)
    if n < 2 then
        return n
    else
        return fib(n-2) + fib(n-1)
    end
end

local function tfib(t, n)
    return tonumber(ffi.cast(t, fib(n)))
end

print(tfib('int8_t', 35))
print(tfib('int16_t', 35))
print(tfib('int32_t', 35))
print(tfib('int64_t', 35))
print(tfib('uint8_t', 35))
print(tfib('uint16_t', 35))
print(tfib('uint32_t', 35))
print(tfib('uint64_t', 35))
