
vm.type = {}

local tags = {}

tags.f64 = vm.typename("f64")
tags.str = vm.typename("str")
tags.tab = vm.typename("tab")

do
    local tags_keys = table.keys(tags)
    for n=1, #tags_keys do
        local key = tags_keys[n]
        local tag = tags[key]
        local vm_type = {}
        vm_type.is_type = true
        vm.type[key] = vm_type
        function vm_type.check(v)
            if vm.typeof(v) ~= tag then
                return { vm.concat("value was not an ", key) }
            end
            return nil
        end
    end
end

local function add_error(table, entry)
    table[#table + 1] = entry
    return table
end

function vm.type.record(checkers)
    local record = {}
    record.is_type = true

    function record.check(tab)
        local c = checkers
        if vm.typeof(tab) ~= tags.tab then
            return { "not a table" }
        end
        local keys = table.keys(c)
        for i=1, #keys do
            local key = keys[i]
            local checker = checkers[key]
            local errors = checker.check(tab[key])
            if errors ~= nil then
                return add_error(errors, "in: record")
            end
        end
        return nil
    end

    return record
end

local function format_error(got, exist, from)
    if got[from] == nil then
        return exist
    else
        return format_error(got, vm.concat(exist, "\n", got[from]), from + 1)
    end
end

local function is_type(obj)
    return vm.typeof(obj) == tags.tab and obj.is_type
end

function vm.check(checker, where, value)
    if not is_type(checker) then
        error("provided type is not actually a type")
    end
    local got = checker.check(value)
    if got ~= nil then
        for i=1, #where do
            add_error(got, where[i])
        end
        error(format_error(got, "type check failed", 1))
    end
end

local t = vm.type.f64

local x2str = vm.type.record({t, t})

-- function foo(a: f64): str
--     return tostring(a)
-- end 

local a_loc = {"in: parameter a", "in: global function to_s"}
local r_loc = {"in: return value", "in: global function to_s"}
local a_type = vm.type.f64
local r_type = vm.type.str

local function to_s(a)
    vm.check(a_type, a_loc, a)
    local r = tostring(a)
    vm.check(r_type, r_loc, r)
    return r
end

print(to_s("x"))
print(to_s(0))
print(to_s(3))

local function fib(n)
    vm.check()
end
