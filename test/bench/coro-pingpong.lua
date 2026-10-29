local total = 0
local gen = coroutine.wrap(function()
    for i = 1, 8000000 do
        coroutine.yield(i)
    end
end)
for i = 1, 8000000 do
    total = total + gen()
end
print(total)
