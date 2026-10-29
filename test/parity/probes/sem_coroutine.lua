local co = coroutine.create(function(a)
    local b = coroutine.yield(a + 1)
    return b * 2
end)
print(coroutine.status(co))
print(coroutine.resume(co, 10))
print(coroutine.status(co))
print(coroutine.resume(co, 5))
print(coroutine.status(co))
