local function run(x, y)
    print('-- ' .. tostring(x) .. ' and ' .. tostring(y))
    print(x and y)
    print(
        do
            local tmp = x
            if x then
                y
            else
                x
            end
        end
    )
end

run(true, true)
run(true, false)
run(false, true)
run(false, false)

