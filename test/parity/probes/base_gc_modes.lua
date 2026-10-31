local keep = {}
for i = 1, 2000 do
    keep[i] = {name = 'item' .. i, list = {i, i + 1}}
end
local finished = 0
for round = 1, 30 do
    collectgarbage('incremental')
    for i = 1, 500 do
        local t = {tostring(i * round)}
        keep[(i * round) % 2000 + 1].extra = t
    end
    if collectgarbage('step', 1) then
        finished = finished + 1
    end
    collectgarbage('generational')
    for i = 1, 500 do
        keep[(i + round) % 2000 + 1].list[3] = 'g' .. i
    end
    collectgarbage('step', 0)
end
collectgarbage('incremental')
while not collectgarbage('step', 0) do
end
collectgarbage('generational')
collectgarbage()
local sum = 0
for i = 1, 2000 do
    local item = keep[i]
    assert(item.name == 'item' .. i)
    sum = sum + item.list[1] + item.list[2] + #(item.extra and item.extra[1] or '')
end
print(sum, finished >= 0)
