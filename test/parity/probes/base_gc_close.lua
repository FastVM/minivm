-- Finalizers of objects still alive at exit run when the state closes, newest
-- first. An error in one doesn't stop the rest, and one set while closing
-- never runs.
local a = setmetatable({}, {__gc = function () print("a") end})
local b = setmetatable({}, {__gc = function () error("x") end})
local c = setmetatable({}, {__gc = function ()
    print("c")
    setmetatable({}, {__gc = function () print("late") end})
end})
print("end", a ~= b, b ~= c)
