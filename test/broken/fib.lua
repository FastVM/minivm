
local function noop()
end

local function pow2(n)
    local r = 0
    if n < 2 then
        return n
    else
        local x = pow2(n - 1)
        local y = pow2(n - 1)
        return x * 2
    end
end

print(pow2(10))
