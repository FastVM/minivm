local function setlocal(...t)
    debug.setlocal(1, 1, 5)
    return select("#", ...), ...
end
print(pcall(setlocal, 1, 2, 3))

local function replace(...t)
    debug.setlocal(1, 1, {7, 8, n = 2})
    return select("#", ...), ...
end
print(pcall(replace, 1, 2, 3))

local function mutate(...t)
    t[1] = 9
    t.n = 1
    return select("#", ...), ...
end
print(pcall(mutate, 1, 2, 3))
