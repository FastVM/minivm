
local count = 0
local max = 10000
local pprime = 2
while pprime < max do
    local check = 2
    local isprime = 1
    while check < pprime do
        if pprime % check == 0 then
            isprime = 0
        end
        check = check + 1
    end
    count = count + isprime
    pprime = pprime + 1
end

print(count)
