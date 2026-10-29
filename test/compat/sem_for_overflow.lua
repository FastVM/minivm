local n = 0
for i = math.maxinteger - 1, math.maxinteger do
    n = n + 1
end
print(n)
n = 0
for i = math.mininteger + 1, math.mininteger, -1 do
    n = n + 1
end
print(n)
n = 0
for i = math.maxinteger - 2, math.maxinteger, 2 do
    n = n + 1
end
print(n)
n = 0
for i = 1, math.huge do
    n = n + 1
    if n == 3 then
        break
    end
end
print(n)
for i = math.maxinteger, math.maxinteger + 1.0 do
    print(math.type(i))
    break
end
for i = 1, 0 do
    print("never")
end
for i = 3, 1 do
    print("never")
end
print(pcall(function() for i = 1, 10, 0 do end end))
print(pcall(function() for i = "a", 2 do end end))
print(pcall(function() for i = 1, {} do end end))
print(pcall(function() for i = 1, 2, "x" do end end))
for i = 1.0, 2 do
    io.write(tostring(i), " ")
end
for i = 1, 2.5 do
    io.write(tostring(i), " ")
end
for i = "1", 2 do
    io.write(tostring(i), " ")
end
print()
