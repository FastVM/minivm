local upto = 100
env.io.debug(2)
local val = 3
while val < upto do
    local check = 3
    local add = 1
    while check * check <= val do
        if val % check == 0 then
            add = 0
        end
        check = check + 2
    end
    if add == 1 then
        env.io.debug(val)
    end
    val = val + 2
end
