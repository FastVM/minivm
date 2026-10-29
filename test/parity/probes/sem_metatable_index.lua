local base = {greet = "hi"}
local t = setmetatable({}, {__index = base})
print(t.greet)
print(getmetatable(t).__index == base)
