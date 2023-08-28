local function out1()
    local function mid1()
        local function in1()
            return 4000
        end
    
        local function in2()
            return 900
        end
    
        return in1() + in2()
    end

    local function mid2()
        local function in3()
            return 80
        end
    
        local function in4()
            return 4
        end
    
        return in3() + in4()
    end
    
    return mid1() + mid2(2)
end
print(out1())