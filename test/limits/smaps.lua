-- Prints the resident size of each mapping of the running interpreter, for
-- finding where baseline RSS goes. Run it with minivm or a reference lua.
local name, rss = nil, 0
local rows = {}
for line in io.lines("/proc/self/smaps") do
    local path = line:match("^%x+%-%x+ %S+ %x+ %S+ %d+%s*(.*)$")
    if path ~= nil then
        if name ~= nil and rss > 0 then
            rows[#rows + 1] = { name, rss }
        end
        if path == "" then
            path = "[anon]"
        end
        name, rss = path, 0
    else
        local kb = line:match("^Rss:%s+(%d+) kB")
        if kb ~= nil then
            rss = rss + tonumber(kb)
        end
    end
end
if name ~= nil and rss > 0 then
    rows[#rows + 1] = { name, rss }
end
local total = 0
for _, row in ipairs(rows) do
    print(string.format("%6d kB  %s", row[2], row[1]))
    total = total + row[2]
end
print(string.format("%6d kB  total", total))
