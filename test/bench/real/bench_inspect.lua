local inspect = require("inspect")
local function build(n, depth)
  local t = {}
  for i = 1, n do
    if depth > 0 then
      t["k" .. i] = build(math.max(1, n // 2), depth - 1)
    else
      t["k" .. i] = {id = i, name = "node_" .. i, val = i * 2}
    end
  end
  return t
end
local doc = build(8, 4)
local iters = 4000
local sum, len = 0, 0
for it = 1, iters do
  local s = inspect(doc)
  len = #s
  if it == iters then
    for i = 1, #s do sum = (sum * 131 + s:byte(i)) % 1000000007 end
  end
end
print("inspect", iters, len, sum)
