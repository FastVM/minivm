local fact
fact = function(n)
    if n <= 1 then
        return 1
    end
    return n * fact(n - 1)
end
print(fact(5))
