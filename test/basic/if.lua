
local v = 1
local run = true
while run do
    local x = if v == 1 then
        "v == 1"
    elseif v == 2 then
        "v == 2"
    elseif v == 3 then
        "v == 3"
    else
        run = false
        "stop"
    end
    print(x)
    v = v + 1
end