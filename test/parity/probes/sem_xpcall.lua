local ok, msg = xpcall(function()
    error("bad", 0)
end, function(e)
    return "handled:" .. e
end)
print(ok, msg)
