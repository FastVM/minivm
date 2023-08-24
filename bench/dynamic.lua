local i = 1000 * 1000 * 1000
local num = 0
local str = ""
local dyn = {}
local n = 0
while i > 0 do
    if i ~= num then
        dyn = num
        n = n + 1
    else
        dyn = str
        n = n - 1
    end
    i = i-1
end
print(n)