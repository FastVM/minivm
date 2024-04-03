
for k,v in pairs(_G) do
    if type(v) == 'function' then
        print(k)
    end
end

for k,v in pairs(_G) do
    if k ~= '_G' then
        if type(v) == 'table' then
            for l, w in pairs(v) do
                if type(w) == 'function' then
                    print(k .. '.' .. l)
                end
            end
        end
    end
end
