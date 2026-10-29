local co = coroutine.create(pcall)
print(coroutine.resume(co, function(a)
    local b = coroutine.yield(a + 1)
    return b * 2
end, 10))
print(coroutine.resume(co, 21))
print(coroutine.status(co))

local direct = coroutine.wrap(coroutine.yield)
print(direct('first', 'pair'))
print(direct('back'))

local failing = coroutine.create(error)
print(coroutine.resume(failing, 'body error', 0))
print(coroutine.status(failing))

local nested = coroutine.create(pcall)
print(coroutine.resume(nested, pcall, coroutine.yield, 'deep'))
print(coroutine.resume(nested, 'resumed'))

local sel = coroutine.wrap(select)
print(sel('#', 1, 2, 3))

local caught = coroutine.create(xpcall)
print(coroutine.resume(caught, function() coroutine.yield(1) error('late', 0) end, function(m) return 'handled ' .. m end))
print(coroutine.resume(caught))

local inner = coroutine.wrap(function() coroutine.yield('in') return 'out' end)
local outer = coroutine.wrap(inner)
print(outer())
print(pcall(outer))
