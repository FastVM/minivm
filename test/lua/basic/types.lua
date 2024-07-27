
local value = nil
local cond = 0

while cond < 3 do
    local mod = cond % 3

    if mod == 0 then
        value = true
    end
    if mod == 1 then
        value = 100
    end
    if mod == 2 then
        value = "hello"
    end
    
    print(value, type(value))
    
    cond = cond + 1
end

