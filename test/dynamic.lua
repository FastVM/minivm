
local function tags()
    std.tagname = std.extern("vm_std_tag")
    std.tag = {}
    std.tag.nil = std.tagname("nil")
    std.tag.bool = std.tagname("bool")
    std.tag.i64 = std.tagname("i64")
    std.tag.f64 = std.tagname("f64")
    std.tag.str = std.tagname("str")
    std.tag.func = std.tagname("fun")
    std.tag.table = std.tagname("tab")
    std.tag.ffi = std.tagname("ffi")
end

tags()

local function type(a)
    if a::type == std.tag.nil then
        return "nil"
    end
    if a::type == std.tag.bool then
        return "bool"
    end
    if a::type == std.tag.i64 then
        return "i64"
    end
    if a::type == std.tag.f64 then
        return "f64"
    end
    if a::type == std.tag.str then
        return "str"
    end
    if a::type == std.tag.table then
        return "tab"
    end
    if a::type == std.tag.func then
        return "fun"
    end
    if a::type == std.tag.ffi then
        return "ffi"
    end
end

local tab = {}
print("str:",  type("hello"))
print("num:",  type(10))
print("tab:",  type(tab))
print("nil:",  type(tab.nope))
