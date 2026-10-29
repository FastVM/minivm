local samples = {
    function(n) ::top:: n = n - 1 if n > 0 then goto top end return n end,
    function(...) while select('#', ...) > 5 do end return ... end,
    function(n) repeat n = n - 1 until n <= 0 return n end,
    function(t) for i = 1, #t do t[i] = t[i] * 2 end return t[1], t[#t] end,
}
local args = {5, 1, 3, {1, 2, 3}}
for i, f in ipairs(samples) do
    for _, strip in ipairs({false, true}) do
        local g = assert(load(string.dump(f, strip), 'dump', 'b'))
        print(i, strip, g(args[i]))
    end
end
