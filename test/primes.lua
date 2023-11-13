
local count = 1
local max = 100000
local pprime = 3
while pprime < max do
    local check = 3
    local isprime = 1
    while check * check <= pprime do
        if pprime%check == 0 then
            isprime = 0
        end
        check = check + 2
    end
    count = count + isprime
    pprime = pprime + 2
end

print(count)
