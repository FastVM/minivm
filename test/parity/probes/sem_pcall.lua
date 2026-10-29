local ok, err = pcall(function()
    error("boom")
end)
print(ok, type(err))
local ok2, v = pcall(function()
    return 42
end)
print(ok2, v)
