
local function pow2(n)
    local bits = 16

    local t = {}
    local n = 0
    while true do
        local c = 1
        while c <= bits do
            if t[c] then
                t[c] = false
            else
                t[c] = true
                break
            end
            c = c + 1
        end
        n = n + 1
        if c > bits then
            break
        end
    end
    return n
end

print(pow2(16))
