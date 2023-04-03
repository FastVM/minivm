
local t = {}
local n = 1000000
while n ~= 0 do
    rawset(t, n, n * n)
    n = n - 1
end
print(rawget(t, 7))
