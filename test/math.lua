
local x = 3
local y = 5

print("x = 3")
print("y = 5")

if x < y then
    print("pass: x < y")
else
    print("fail: x < y")
end

if x < 3 then
    print("fail: x < 3")
else
    print("pass: x < 3")
end

if 3 < x then
    print("fail: 3 < x")
else
    print("pass: 3 < x")
end

if 3 < 3 then
    print("fail: 3 < 3")
else
    print("pass: 3 < 3")
end

if x < 5 then
    print("pass: x < 5")
else
    print("fail: x < 5")
end

if 3 < y then
    print("pass: 3 < y")
else
    print("fail: 3 < y")
end

if 3 < 5 then
    print("pass: 3 < 5")
else
    print("fail: 3 < 5")
end