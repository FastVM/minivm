local acc = {}
for w in string.gmatch("one two three", "%a+") do
    acc[#acc + 1] = w
end
print(table.concat(acc, "|"))
