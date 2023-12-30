
local count = 1
for pprime=3, 1000000, 2 do
    local check = 3
    while check * check <= pprime do
        if pprime % check == 0 then
            count = count - 1
            break
        end
        check = check + 2
    end
    count = count + 1
end

return count
