-- The Computer Language Benchmarks Game
-- http://benchmarksgame.alioth.debian.org/
-- contributed by Mike Pall
local function BottomUpTree(item, depth)
    if depth > 0 then
        local i = item + item
        depth = depth - 1
        local left = BottomUpTree(i - 1, depth)
        local right = BottomUpTree(i, depth)
        return {item, left, right}
    else
        return {item}
    end
end

local function ItemCheck(tree)
    if #tree == 3 then
        return tree[1] + ItemCheck(tree[2]) - ItemCheck(tree[3])
    else
        return tree[1]
    end
end

local function pow2(n)
    if n == 0 then
        return 1
    else
        return pow2(n - 1) * 2
    end
end

local mindepth = 4
local maxdepth = tonumber(arg and arg[1]) or 16

local stretchdepth = maxdepth + 1

local stretchtree = BottomUpTree(0, stretchdepth)

local longlivedtree = BottomUpTree(0, maxdepth)

local depth = mindepth
while depth <= maxdepth do
    local iterations = pow2(maxdepth - depth + mindepth)
    local check = 0
    local i = 1
    while i <= iterations do
        local x = ItemCheck(BottomUpTree(1, depth)) 
        check = check + x + ItemCheck(BottomUpTree(-1, depth))
        i = i + 1
    end
    print(depth, check)
    depth = depth + 2
    -- if vm then vm.gc() end
end

print(maxdepth, ItemCheck(longlivedtree))
