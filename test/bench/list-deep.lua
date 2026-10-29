-- deep linked structure: trips C-stack recursion in the GC trace and
-- measures pointer-chasing walk speed
local head = nil
for i = 1, 2000000 do
    head = {next = head, v = i}
end
collectgarbage("collect")
local n = 0
local p = head
while p do
    n = n + 1
    p = p.next
end
print(n)
