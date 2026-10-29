-- a label is visible only inside its own block, so sibling blocks may reuse
-- the name; a goto still may not reach into a block that has closed.
for q = 1, 2 do ::cont:: end
for q = 1, 2 do ::cont:: end
print("siblings")

local n = 0
for q = 1, 4 do
  if q % 2 == 0 then goto cont end
  n = n + q
  ::cont::
end
print(n)

do
  local m = 0
  for q = 1, 3 do
    for r = 1, 3 do
      if r == 2 then goto inner end
      m = m + 1
      ::inner::
    end
    ::outer::
  end
  print(m)
end

do goto out end
::out::
print("forward")

print(load("do ::a:: ::a:: end") == nil)
print(load("do ::x:: end do goto x end") == nil)
print(load("for i = 1, 1 do ::z:: end for i = 1, 1 do ::z:: end") ~= nil)
