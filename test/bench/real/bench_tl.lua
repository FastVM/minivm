local tl = require("tl")
local prog = [==[
local record Point
  x: number
  y: number
end
local function dist(a: Point, b: Point): number
  local dx = a.x - b.x
  local dy = a.y - b.y
  return (dx*dx + dy*dy)
end
local function make(n: number): {Point}
  local out: {Point} = {}
  for i = 1, n do
    out[i] = { x = i, y = i * 2 }
  end
  return out
end
local pts = make(10)
local total = 0.0
for i = 2, #pts do
  total = total + dist(pts[i-1], pts[i])
end
print(total)
]==]
local iters = 100
local last
for it = 1, iters do
  last = tl.gen(prog)
end
local sum = 0
for i = 1, #last do sum = (sum * 131 + last:byte(i)) % 1000000007 end
print("tl", iters, #last, sum)
