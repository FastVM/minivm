local function closer()
    return setmetatable({}, {__close = function() print('closed') end})
end
local function three() return 1, 2, 3 end
local samples = {
    function() local c <close> = closer() return three() end,
    function(...) local c <close> = closer() return ... end,
    function(n) ::top:: n = n - 1 if n > 0 then goto top end return n end,
    function(...) ::top:: local n = select('#', ...) if n > 9 then goto top end return n, ... end,
    function(...t) return t.n, ... end,
    function(a, ...) local t = {a, ...} return #t end,
}
for i, f in ipairs(samples) do
    for _, strip in ipairs({false, true}) do
        local g = assert(load(string.dump(f, strip), 'dump', 'b'))
        if debug.getupvalue(g, 1) ~= nil then
            debug.upvaluejoin(g, 1, f, 1)
        end
        if debug.getupvalue(g, 2) ~= nil then
            debug.upvaluejoin(g, 2, f, 2)
        end
        print(i, strip, g(3, 'x', nil))
    end
end
