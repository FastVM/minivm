local acc = {}
for i = 10, 1, -3 do
    acc[#acc + 1] = i
end
print(table.concat(acc, ","))
