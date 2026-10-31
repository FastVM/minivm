local held = {}
local churn = {}
for i = 1, 20000 do
    churn[i] = {}
end
held.t = {}
held.f = function() end
held.u = io.stdout
for i = 20001, 40000 do
    churn[i] = {}
end
local t = tostring(held.t)
local f = tostring(held.f)
local p = string.format('%p', held.t)
churn = nil
for _ = 1, 4 do
    collectgarbage()
end
print(t == tostring(held.t), f == tostring(held.f), p == string.format('%p', held.t))
