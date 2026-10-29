-- mutate a large tenured table while allocating: exercises the card-marking
-- write barrier and dirty-block rescans across minor collections
local big = {}
for i = 1, 1000000 do
    big[i] = i
end
collectgarbage("collect")
collectgarbage("collect")
local n = 0
for i = 1, 3000000 do
    big[(i % 1000000) + 1] = i
    local u = {i}
    n = n + u[1]
end
print(n)
