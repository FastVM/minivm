
local function box(n)
    local tab = {
        value = n
    }
    
    function tab:add(other)
        print(self.value)
        return box(self.value+other)
    end

    function tab:mul(other)
        print(self.value)
        return box(self.value*other)
    end

    function tab:echo()
        print(self.value)
    end

    return tab
end

box(3):mul(4):add(1):echo()
