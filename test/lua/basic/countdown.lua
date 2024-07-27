
local function gen(n)
	local function dec(count)
		return {
			count,
			function()
				return dec(count - 1)
			end
		}
	end
	return dec(n)
end

local function countdown(counter)
	if counter[1] > 0 then
		print(counter[1])
		return countdown(counter[2]())
	end
end

countdown(gen(5))
