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
    if tree[2] then
        return tree[1] + ItemCheck(tree[2]) - ItemCheck(tree[3])
    else
        return tree[1]
    end
end

local stretchtree = BottomUpTree(0, 12)
print(stretchdepth, ItemCheck(stretchtree))
