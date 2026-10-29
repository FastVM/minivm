local function show(label, ...)
    local t = table.pack(...)
    for i = 1, t.n do
        t[i] = tostring(t[i])
    end
    print(label, table.concat(t, " | ", 1, t.n))
end

show("tonumber embedded zero", tonumber("0xffffffffffffffff\0"), tonumber("10\0"))
show("pcall as coroutine body", coroutine.resume(coroutine.create(pcall), function () coroutine.yield(1) end))
show("__len arguments", (function ()
    local n
    local a = setmetatable({}, {__len = function (...) n = select("#", ...); return 0 end})
    local _ = #a
    return n
end)())
do
    local c = {}
    for i = 1, 200 do
        c[#c + 1] = ";global x" .. i
        c[#c + 1] = ";local y" .. i .. "=" .. i
    end
    local f, e = load(table.concat(c))
    show("200 locals plus globals", f ~= nil, e)
end
do
    local s, line = load("return [[\nalo\ralo\n\n]], debug.getinfo(1).currentline")()
    show("CR in long string", #s, line)
end
show("<close> argument count", (function ()
    local n
    do
        local a <close> = setmetatable({}, {__close = function (...) n = select("#", ...) end})
    end
    return n
end)())
show("for below mininteger", (function ()
    local c = 0
    for i = math.mininteger, -10e100 do
        c = c + 1
        break
    end
    return c
end)())
show("gmatch init past end", (function ()
    local s = 0
    for k in string.gmatch("11 21 31", "%w*", 10) do
        s = s + 1
    end
    return s
end)())
show("utf8.offset continuation", pcall(utf8.offset, "\x9c", -1))
show("vararg bad n", pcall(function (a, b, n, ...t) t.n = n; return b, ... end, 1, 1, -1))
show("load mode B", pcall(load, "", "", "B"))
show("packsize huge", pcall(string.packsize, string.format("c%d", math.maxinteger - 9)))
show("rep interned", string.format("%p", string.rep("a", 10)) == string.format("%p", string.rep("aa", 5)))
show("stripped activelines", #debug.getinfo(load(string.dump(load("print(10)"), true)), "L").activelines)
show("math.huge << 1", pcall(load("return math.huge << 1")))
show("__add not callable", pcall(load("local a = setmetatable({}, {__add = 34}); a = a + 1")))
show("searchpath", package.searchpath("xuxu", "./?.x;/no/??.y"))
do
    local t = {}
    for i = 1, 2000 do
        t[i] = string.rep("?", i % 10 + 1)
    end
    t[#t + 1] = ";"
    local s, err = package.searchpath("xuxu", table.concat(t, ";"))
    show("searchpath 2000 templates", s, string.find(err, string.rep("xuxu", 10)) ~= nil, #string.gsub(err, "[^\n]", ""))
end
show("table.create huge hash", pcall(table.create, 0, 1 << 31))
show("table.create huge", pcall(table.create, 1 << 31))
show("yield in __close of self-close", coroutine.resume(coroutine.create(function ()
    local x <close> = setmetatable({}, {__close = function () coroutine.yield() end})
    string.gsub("a", "a", function ()
        pcall(pcall, function ()
            coroutine.close()
        end)
    end)
end)))
show("yield in __close of self-close, yieldable", coroutine.resume(coroutine.create(function ()
    local x <close> = setmetatable({}, {__close = function () coroutine.yield() end})
    coroutine.close()
end)))
