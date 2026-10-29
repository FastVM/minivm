function is_even(n)
    if n == 0 then
        return 1
    end
    return is_odd(n - 1)
end

function is_odd(n)
    if n == 0 then
        return 0
    end
    return is_even(n - 1)
end

local count = 0
local i = 0
while i < 30000 do
    count = count + is_even(i % 1000 + 1000)
    i = i + 1
end

print(count)
