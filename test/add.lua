local function add(x, y)
    if x == 0 then
        return y
    else
        return add(x-1, y+1)
    end
end

print(add(10, 20))
