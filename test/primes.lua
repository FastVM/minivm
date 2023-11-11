local count = 1
local max = 1000000
local pprime = 3
while pprime < max do
    local check = 3
    local isprime = 1
    while check * check <= pprime do
        if pprime%check == 0 then
            isprime = 0
        end
        check = check + 1
    end
    count = count + isprime
    pprime = pprime + 2
end

print(count)
print([["PRIMES"]])