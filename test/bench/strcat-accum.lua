local s = ""
for i = 1, 100000 do
    s = s .. "xy"
end
print(#s)
print(s:sub(1, 4), s:sub(-2))
