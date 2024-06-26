local function tarai(x, y, z)
    if y < x then
        return tarai( 
            tarai(x-1, y, z),
            tarai(y-1, z, x),
            tarai(z-1, x, y)
        )
    else
        return y
    end
end

print(tarai(20, 19, 32))
