local json = require("json")
local function build(n)
  local t = {meta = {count = n, name = "bench"}, items = {}}
  for i = 1, n do
    t.items[i] = {
      id = i, name = "item_" .. i,
      tags = {"a", "b", "c", tostring(i % 7)},
      active = (i % 2 == 0), score = i * 3,
      nested = {x = i, y = i + 1, z = {deep = i * 2}},
    }
  end
  return t
end
local doc = build(2000)
local iters = 60
local acc = 0
for it = 1, iters do
  local s = json.encode(doc)
  acc = (acc + #s) % 1000000007  -- length is order-independent for this fixed schema
  local d = json.decode(s)
  -- order-independent numeric invariant from decoded data
  local chk = 0
  for i = 1, #d.items do
    local it2 = d.items[i]
    chk = (chk + it2.id * 31 + it2.score * 7 + it2.nested.z.deep) % 1000000007
  end
  acc = (acc + chk) % 1000000007
end
print("json", iters, acc)
