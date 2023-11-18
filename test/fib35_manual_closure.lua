
local fib = vm.closure(function(c, n)
    if n < 2 then
        return n
    else
        return c[0](c, n-1) + c[0](c, n-2)
    end
end)

print(fib[0](fib, 35))
