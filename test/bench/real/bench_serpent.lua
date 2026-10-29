local serpent = require("serpent")
local function build(n, depth)
  local t = {}
  for i = 1, n do
    if depth > 0 then
      t["k" .. i] = build(math.max(1, n // 2), depth - 1)
    else
      t["k" .. i] = {id = i, name = "node_" .. i, vals = {i, i * 2, i * 3.5, true, "s" .. i}}
    end
  end
  return t
end
local doc = build(8, 4)
local iters = 800
local sum, len = 0, 0
for it = 1, iters do
  local s = serpent.block(doc, {comment = false, sortkeys = true})
  local ok, back = serpent.load(s)
  if not ok then error("roundtrip failed") end
  len = #s + (back.k1 and 1 or 0)
  if it == iters then
    for i = 1, #s do sum = (sum * 131 + s:byte(i)) % 1000000007 end
  end
end
print("serpent", iters, len, sum)
