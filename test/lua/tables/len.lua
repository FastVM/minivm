local function check(t)
    local len = #t
    assert(len % 1 == 0, "length must be an integer")
    assert(len >= 0, "length must be nonnegative") -- is this a guarantee, an implementation detail, or can be broken
    assert(len == 0 or t[len] == nil, "value at index length must be occupied or length must be zero")
    assert(t[len + 1] == nil, "length + 1 should not be in the table")
end

local function checks()
    for i=1, 256 do
        local c = i
        local t = {}
        for j=0, 7 do
            if c % 2 ~= 0 then
                t[j] = '<do-not-care>'
            end
            c = c // 2
        end
        check(t)
    end
end
  
