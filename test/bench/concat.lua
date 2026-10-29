local total = 0
local i = 0
while i < 400000 do
    local s = ""
    local j = 0
    while j < 10 do
        s = s .. tostring(j)
        j = j + 1
    end
    assert(s == "0123456789", "bad concat")
    total = total + string.len(s)
    i = i + 1
end

print(total)
