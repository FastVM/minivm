local size = 600
local sum = 0
for y = 0, size - 1 do
    local ci = 2 * y / size - 1
    for x = 0, size - 1 do
        local cr = 2 * x / size - 1.5
        local zr, zi = 0, 0
        local n = 0
        while n < 49 do
            local zr2 = zr * zr
            local zi2 = zi * zi
            if zr2 + zi2 > 4 then
                break
            end
            zi = 2 * zr * zi + ci
            zr = zr2 - zi2 + cr
            n = n + 1
        end
        if n == 49 then
            sum = sum + 1
        end
    end
end
print(sum)
