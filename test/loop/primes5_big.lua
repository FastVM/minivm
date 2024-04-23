
local count = 1
local max = 10000000
local pprime = 3
while pprime < max do
    local check = 3
    while check * check <= pprime do
        if pprime % check == 0 then
            count = count - 1
            break
        end
        check = check + 2
    end
    count = count + 1
    pprime = pprime + 2
end

print(count)
