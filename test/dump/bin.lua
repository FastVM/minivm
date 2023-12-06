

local lib = {}

function lib.new(obj)
    return vm.dumpbin(-10)
end

function lib.bytes(tab)
    local n = 1
    while n <= #tab do
        print(tab[n])
        n = n + 1
    end
end

local obj = lib.new(10)
lib.bytes(obj)
