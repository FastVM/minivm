local function BottomUpTree(item, depth)
    if depth > 0 then
        local i = item + item
        depth = depth - 1
        local left = BottomUpTree(i - 1, depth)
        local right = BottomUpTree(i, depth)
        return {item, left, right}
    end
    return {item}
end
local function ItemCheck(tree)
    if #tree == 3 then
        return tree[1] + ItemCheck(tree[2]) - ItemCheck(tree[3])
    end
    return tree[1]
end
local mindepth = 4
local maxdepth = 12
local stretch = BottomUpTree(0, maxdepth + 1)
print("stretch", ItemCheck(stretch))
local longlived = BottomUpTree(0, maxdepth)
for depth = mindepth, maxdepth, 2 do
    local iterations = 2 ^ (maxdepth - depth + mindepth)
    local check = 0
    for i = 1, iterations do
        check = check + ItemCheck(BottomUpTree(1, depth)) + ItemCheck(BottomUpTree(-1, depth))
    end
    print(depth, check)
end
print("longlived", ItemCheck(longlived))
