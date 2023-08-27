
local function type(a)
    if a::type == env.tag.nil then
        return "nil"
    end
    if a::type == env.tag.bool then
        return "bool"
    end
    if a::type == env.tag.i64 then
        return "i64"
    end
    if a::type == env.tag.f64 then
        return "f64"
    end
    if a::type == env.tag.str then
        return "str"
    end
    if a::type == env.tag.table then
        return "tab"
    end
    if a::type == env.tag.func then
        return "fun"
    end
    if a::type == env.tag.ffi then
        return "ffi"
    end
end

local tab = {}
print("str:",  type("hello"))
print("num:",  type(10))
print("tab:",  type(tab))
print("nil:",  type(tab.nope))
