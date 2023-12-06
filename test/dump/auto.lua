

local lib = {}

function lib.new(obj)
    local t = {}
    local function put(byte)
        t[#t + 1] = byte
    end

    local function more(obj)
        
    end
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
