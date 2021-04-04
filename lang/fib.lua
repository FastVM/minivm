local function fib(self, n)
    if n < 2 then
        return n
    else
        return self(self, n-1) + self(self, n-2)
    end
end

print(fib(fib, 40))