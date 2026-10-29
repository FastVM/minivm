local t = {3, 1, 2}
table.sort(t)
print(table.concat(t, ","))
table.sort(t, function(a, b)
    return a > b
end)
print(table.concat(t, ","))
