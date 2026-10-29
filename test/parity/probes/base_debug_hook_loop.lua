debug.sethook(function() error('budget') end, '', 1000)
print(pcall(function() ::a:: goto a end))
print(pcall(function() while true do end end))
print(pcall(function() repeat until false end))
print(pcall(function() for _ = 1, math.huge do end end))
debug.sethook()

local lines = {}
local function loop()
    local n = 0
    ::a::
    n = n + 1
    goto a
end
debug.sethook(function(_, line)
    if debug.getinfo(2, 'f').func == loop then
        lines[#lines + 1] = line
        if #lines == 7 then
            error('lines')
        end
    end
end, 'l')
print(pcall(loop))
debug.sethook()
print(table.concat(lines, ' '))
