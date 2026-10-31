local function leaf()
    local here = debug.getinfo(1, 't')
    local caller = debug.getinfo(2, 't')
    return here.istailcall, caller.istailcall
end

local function tail()
    return leaf()
end

local function plain()
    local a, b = leaf()
    return a, b
end

print(tail())
print(plain())
print(debug.getinfo(1, 't').istailcall)

local function boom()
    error('boom')
end

local function via()
    return boom()
end

print(select(2, xpcall(via, debug.traceback)))
