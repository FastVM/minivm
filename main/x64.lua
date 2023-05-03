
local dis = require('main.dis')

local function slurp(filename)
    local f = io.open(filename)
    local r = f:read('*all')
    f:close()
    return r
end

local function dump(name, data)
    local val = io.open(name, "w")
    val.write(val, data)
    val:close()
end

local src = slurp(arg[1])
print('--- ' .. arg[1] .. ' ---')
dis.disass64(src)
