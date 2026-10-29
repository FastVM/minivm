local t = {}
t["alpha"] = 0
t["beta"] = 0
t["gamma"] = 0
t["delta"] = 0
t["epsilon"] = 0

local i = 0
while i < 10000000 do
    local m = i % 5
    if m == 0 then
        t["alpha"] = t["alpha"] + 1
    elseif m == 1 then
        t["beta"] = t["beta"] + 2
    elseif m == 2 then
        t["gamma"] = t["gamma"] + 3
    elseif m == 3 then
        t["delta"] = t["delta"] + 4
    else
        t["epsilon"] = t["epsilon"] + 5
    end
    i = i + 1
end

local total = t["alpha"] + t["beta"] + t["gamma"] + t["delta"] + t["epsilon"]
print(total)
