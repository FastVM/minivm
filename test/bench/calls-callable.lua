local mt_add = {__call = function(self, x) return x + self.k end}
local mt_mul = {__call = function(self, x) return (x * self.k) % 1048576 end}

local objs = {
    setmetatable({k = 3}, mt_add),
    setmetatable({k = 2}, mt_mul),
    setmetatable({k = 7}, mt_add),
    setmetatable({k = 5}, mt_mul),
}

local mono = setmetatable({n = 0}, {__call = function(self, x)
    self.n = (self.n + x) % 1048576
    return self.n
end})

local acc = 1
local i = 0
while i < 4000000 do
    local f = objs[i % 4 + 1]
    acc = f(acc)
    acc = mono(acc)
    i = i + 1
end

print(acc, mono.n)
