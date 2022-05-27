local function isprime(num)
    local check = 3
    while check * check <= num do
        if num % check == 0 then
            return 0
        end
        check = check + 2
    end 
    return 1
end

local function primes(upto)
    local count = 2
    local val = 5
    while val < upto do
        count = count + isprime(val)
        val = val + 2
    end
    print(count)
end

primes(1000000)
