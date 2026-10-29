local a = 0
local b = 0
local c = 0
local d = 0
local x = 12345
local i = 0
while i < 20000000 do
    x = (x * 16807) % 2147483647
    local m = x % 16
    if m < 3 then
        a = a + 1
    elseif m < 7 then
        b = b + 1
    elseif m < 12 then
        c = c + 1
    else
        d = d + 1
    end
    i = i + 1
end

print(a)
print(b)
print(c)
print(d)
print(a + b + c + d)
