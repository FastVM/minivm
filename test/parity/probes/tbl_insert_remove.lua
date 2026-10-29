local t = {1, 2, 3}
table.insert(t, 4)
table.insert(t, 1, 0)
print(table.concat(t, ","))
local r = table.remove(t)
local r1 = table.remove(t, 1)
print(r, r1, table.concat(t, ","))
