

local table = table

if type(table) == 'nil' then
    table = {}
end

function table.concat(table)
    local function more(low, high)
        if low + 1 >= high then
            return table[low]
        else
            local mid = (high + low) // 2
            return more(low, mid) .. more(mid, high)
        end
    end

    return more(1, #table + 1)
end

print(table.concat({'a', 'b'}))