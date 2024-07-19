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

local function more(pow)
    if pow == 0 then
        return ItemCheck(BottomUpTree(0, 12))
    else
        local next = pow - 1
        local a = more(next)
        local b = more(next)
        return a + b
    end
end

print(more(10))
