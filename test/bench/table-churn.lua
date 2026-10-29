-- distinct-key insert+delete churn: trips tombstone-driven hash growth
-- (capacity must track live entries, not dead ones)
local t = {}
for i = 1, 6000000 do
    local k = "k" .. i
    t[k] = i
    t[k] = nil
end
print(next(t) == nil)
