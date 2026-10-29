local t
local ok, err = pcall(function() return t.x end)
print((err:gsub("^.*:%d+: ", "")))
local up
local function inner() return up.field end
local ok2, err2 = pcall(inner)
print((err2:gsub("^.*:%d+: ", "")))
