
local fill_value = "<do-not-care>"
local max = 100

local ft = {}

local fi = 1
while fi < max do
    ft[fi] = fill_value
    fi = fi + 1
end

assert(#ft == max - 1, "forward table")

local bt = {}
local bi = max
while bi > 1 do
    assert(#bt == 0, "backward table")
    bt[bi] = fill_value
    bi = bi - 1
end
bt[1] = fill_value
assert(#bt == max, "backward table")

local mt = {}
local mi = 1
while mi < max - mi do
    mt[mi] = fill_value
    mt[max - mi] = fill_value
    assert(#mt == mi, "mid table")
    mi = mi + 1
end
mt[mi] = fill_value
assert(#mt == max - 1, "mid table")

local function make_dt()
    local ret = {}
    local i = 1
    while i < max do
        ret[i] = fill_value
        i = i + 1
    end
    return ret
end

local di = 1
while di < max do
    local dt = make_dt()
    assert(#dt == max - 1, "delete table")
    dt[di] = nil
    assert(#dt == di - 1, "delete table")
    di = di + 1
end
