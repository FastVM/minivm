
local function numtype()
    if 1 / 2 == 0 then
        local a = 128
        if a < 0 then
            return "i8"
        end
        local b = 32768
        if b < 0 then
            return "i16"
        end
        local c = 2147483648
        if c < 0 then
            return "i32"
        end
        return "i64"
    else
        if 1000 / 0.001 < 1000000 then
            return "f32"
        else
            return "f64"
        end
    end
end

print(numtype())
