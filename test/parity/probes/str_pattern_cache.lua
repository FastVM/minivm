local pats = {}
for i = 1, 40 do
    pats[#pats + 1] = "(%a+)" .. string.rep("%d", i % 4) .. "()"
end
local s = "abc1234 de56 f7 ghij"
for round = 1, 3 do
    local out = {}
    for _, p in ipairs(pats) do
        local w, at = s:match(p)
        out[#out + 1] = tostring(w) .. ":" .. tostring(at)
    end
    print(round, table.concat(out, " ", 1, 8))
end
print(("^abc"):find("^a"), ("x^abc"):find("^a"), ("abc"):find("b", 1))
for _ = 1, 2 do
    print(pcall(string.match, "x", "[a"))
    print(pcall(string.find, "x", "%"))
end
local seen = {}
local r = s:gsub("%a+", function(w)
    for _, p in ipairs(pats) do
        w:match(p)
    end
    seen[#seen + 1] = w
    return w:upper()
end)
print(r, table.concat(seen, ","))
print(("a.b.c"):gsub("%.", "/"), ("a.b.c"):gsub("%.", "/"))
print(("hello world"):gsub("o", {o = "0"}))
for k, v in ("a=1, b=2"):gmatch("(%w+)=(%w+)") do
    print(k, v, ("k" .. k):match("^k(%a)$"))
end
