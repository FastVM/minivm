
local function tree_new(item, depth)
    if depth > 0 then
        local ret = {}
        rawset(ret, 1, item)
        local i = item + item
        depth = depth - 1
        rawset(ret, 2, tree_new(i-1, depth))
        rawset(ret, 3, tree_new(i, depth))
        return ret
    else
        local ret = {}
        rawset(ret, 1, item)
        return ret
    end
end

local function item_check(tree)
    if #tree == 3 then
        return rawget(tree, 1) + item_check(rawget(tree, 2)) - item_check(rawget(tree, 3))
    else
        return rawget(tree, 1)
    end
end

local function pow2(n)
    if n == 0 then
        return 1
    else
        return pow2(n-1) * 2
    end
end

local maxdepth = 16

print(item_check(tree_new(0, maxdepth+1)))

local long_lived = tree_new(0, maxdepth)

local depth = 4
while depth < maxdepth do
    local maxiter = pow2(maxdepth - depth + 4)
    local check = 0
    while maxiter > 0 do
        check = check + item_check(tree_new(0, depth)) + item_check(tree_new(1, depth))
        maxiter = maxiter - 1
    end
    print(check)
    depth = depth + 2
end

print(item_check(long_lived))