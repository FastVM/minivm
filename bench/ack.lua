local function ackermann(m, n)
    while m ~= 0 do
        if n ~= 0 then
            n = ackermann(m, n-1)
        else
            n = 1
        end
        m = m - 1
    end
    return n+1
end
print(ackermann(3, 11))
