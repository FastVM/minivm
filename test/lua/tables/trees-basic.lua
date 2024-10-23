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
        local v1 = tree[1]
        local v2 = ItemCheck(tree[2])
        local v3 = ItemCheck(tree[3])
        print(v1, v2, v3)
        return v1 + v2 - v3
    else
        print(tree[1])
        return tree[1]
    end
end

local stretchtree = BottomUpTree(0, 3)
print(stretchdepth, ItemCheck(stretchtree))
