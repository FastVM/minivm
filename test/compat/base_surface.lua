local out = {}
for k, v in pairs(_G) do
    if type(v) == "table" and k ~= "_G" and k ~= "package" and k ~= "arg" then
        for kk, vv in pairs(v) do
            out[#out + 1] = k .. "." .. tostring(kk) .. ":" .. type(vv)
        end
    elseif k ~= "arg" then
        out[#out + 1] = k .. ":" .. type(v)
    end
end
for kk, vv in pairs(package) do
    out[#out + 1] = "package." .. kk .. ":" .. type(vv)
end
table.sort(out)
print(table.concat(out, "\n"))
