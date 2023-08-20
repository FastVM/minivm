
local nprimes = 1
local p = 3
while p < 1000000 do
    local check = 1
    local isprime = 1
    while check * check < p do
        check = check + 2
        if p % check < 1 then
            isprime = 0
        end
    end
    nprimes = nprimes + isprime
    p = p + 2
end
print(nprimes)
