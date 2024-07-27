
local fib = function(fib, n)
    if n < 2 then
        return n
    else
        return fib(fib, n-2) + fib(fib, n-1)
    end
end

print(fib(fib, tonumber(arg and arg[1]) or 35))
