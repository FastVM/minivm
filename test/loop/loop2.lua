
local loops = 0
local iters = tonumber(arg and arg[1]) or 100

local a = 0
while a < iters do
    local b = 0
    while b < iters do
        local c = 0
        while c < iters do
            local d = 0
            while d < iters do
                d = d + 1
                loops = loops + 1
            end
            c = c + 1
        end
        b = b + 1
    end
    a = a + 1
end

print(loops)
