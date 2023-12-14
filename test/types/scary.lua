
local function cond(n)
    if n % 2 == 0 then
        return ""
    else
        return 0
    end
end

local t = {}
local x = 0
while x < 256 do
    local v0 = cond(x)
    local v1 = cond(x / 2)
    local v2 = cond(x / 4)
    local v3 = cond(x / 8)
    local v4 = cond(x / 16)
    local v5 = cond(x / 32)
    local v6 = cond(x / 64)
    local v7 = cond(x / 128)
    t[x] = {v0, v1, v2, v3, v4, v5, v6, v7}
    x = x + 1
end

