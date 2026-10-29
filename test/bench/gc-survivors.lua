local function build_list(seed, len)
    local head = nil
    local j = 0
    while j < len do
        head = {seed + j, head}
        j = j + 1
    end
    return head
end

local function walk_sum(node)
    local s = 0
    while node ~= nil do
        s = s + node[1]
        node = node[2]
    end
    return s
end

local window = {}
local wsize = 8
local listlen = 1000
local iters = 3000

local checksum = 0
local i = 0
while i < iters do
    local slot = i % wsize + 1
    window[slot] = build_list(i, listlen)
    checksum = checksum + walk_sum(window[slot])
    i = i + 1
end

local live = 0
local k = 1
while k <= wsize do
    live = live + walk_sum(window[k])
    k = k + 1
end

print(checksum)
print(live)
