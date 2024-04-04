
local fib_cap = {}
local fib = function(closure, n)
    if n < 2 then
        return n
    else
        return closure:fib(n-2) + closure:fib(n-1)
    end
end

fib_cap.fib = fib

local res = fib(fib_cap, tonumber(arg and arg[1]) or 35))
print(res)
