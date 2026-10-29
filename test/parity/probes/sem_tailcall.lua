local function count(n) if n == 0 then return 0 end return count(n - 1) end
print(count(1000000))
local function even(n) if n == 0 then return true end return odd(n - 1) end
function odd(n) if n == 0 then return false end return even(n - 1) end
print(even(100), odd(100), even(99), odd(99))
