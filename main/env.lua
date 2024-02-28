
local function more(tab, indent, path, done)
    for i=1, #done do
        if done[i] == tab then
            return
        end
    end
    done[#done + 1] = tab
    local prefix = indent .. path
    if type(tab) == 'table' then
        print(prefix .. ': ' .. 'table {')
        for k,v in pairs(tab) do
            if type(k) == 'string' then
                more(v, indent .. '  ', k, done)
            elseif type(k) == 'number' then
                more(v, indent .. '  ', '[' .. tostring(k) .. ']', done)
            else
                print(k)
            end
        end
    elseif type(tab) == 'function' then
        print(prefix .. ': ' .. 'function')
    elseif type(tab) == 'string' then
        print(prefix .. ': ' .. 'string')
    elseif type(tab) == 'number' then
        print(prefix .. ': ' .. tostring(tab))
    else
        print(tab)
    end
    done[#done] = nil
end

more(_G, '', '_ENV', '_G', {})
