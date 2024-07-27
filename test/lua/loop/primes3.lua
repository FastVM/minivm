
local count = 1
local max = 1000000
local pprime = 3
while pprime < max do
    local check = 3
    local isprime = true
    while check * check <= pprime do
        if pprime % check == 0 then
            isprime = false
            break
        end
        check = check + 2
    end
    if isprime then
        count = count + 1
    end
    pprime = pprime + 2
end

print(count)
