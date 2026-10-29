local ok, err = pcall(function()
    error({code = 7})
end)
print(ok, type(err), err.code)
