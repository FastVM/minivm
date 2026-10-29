local c = 1
do
    local c <const> = 2
    print(c)
end
c = 3
print(c)
local function f()
    c = 4
    do
        local c <close> = nil
    end
    c = 5
    return c
end
print(f(), c)
print(load("local k <const> = 1; do local k = 2; k = 3 end; k = 4"))
print(load("local k <const> = 1; local function g() k = 2 end"))
print(type(load("local k = 1; do local k <const> = 2 end; local function g() k = 2 end return 1")))
for i = 1, 1 do
    local i = i + 1
    i = i * 2
    print(i)
end
