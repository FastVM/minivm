-- a for-loop control variable is const only inside its loop; once the scope
-- closes the name is an ordinary global again.
for i = 1, 1 do end
i = 5
print(i)

for j = 1, 1 do end
local j = 7
j = 8
print(j)

do
  local k <const> = 1
  print(k)
end
k = 2
print(k)

-- still rejected where the binding is live
print(load("local x <const> = 1; x = 2") == nil)
print(load("for i = 1, 1 do i = 2 end") == nil)
