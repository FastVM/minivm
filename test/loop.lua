local start = os.clock()

local i = 0
local n = 0

while i < 1000000 do
    i = i + 1
    n = n + i
end

local stop = os.clock()

print(string.format('%.3fms', (stop - start) * 1000))
