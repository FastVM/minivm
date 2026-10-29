local dir = arg[1]
package.cpath = dir .. '/?.so'

local function check(name, got, want)
    if got ~= want then
        error(string.format('%s: got %s, want %s', name, tostring(got), tostring(want)), 2)
    end
end

local luamod = require('luamod')
check('add', luamod.add(2, 3), 5)
check('concat', luamod.concat('a'), 'a!')
check('concat2', luamod.concat('a', 'b'), 'ab')
local step = luamod.counter()
step()
check('counter', step(), 2)
check('sum', luamod.sum({1, 2, 3, 4}), 10)
check('ptr', type(luamod.ptr()), 'userdata')
local p = luamod.point(3, 4)
check('point.sum', p:sum(), 7)
check('point.uv', p:uv(), 'tag')
check('point.tostring', tostring(p), 'Point(3,4)')

local ok, err = pcall(luamod.boom)
check('boom.ok', ok, false)
check('boom.msg', err, 'boom from C')
ok, err = pcall(luamod.add, 1, 'x')
check('argerror.ok', ok, false)
check('argerror.msg', err:match("bad argument #2 to '.-' %(number expected, got string%)") ~= nil, true)
ok, err = pcall(p.sum, {})
check('checkudata.ok', ok, false)

local json = require('luajson')
check('json.version', json._VERSION, 'luajson 1.0')
check('json.encode', json.encode({1, 2, 'x', true}), '[1,2,"x",true]')
local decoded = json.decode('{"a": [1, 2.5, "s\\n"], "b": null, "c": false}')
check('json.decode.a1', decoded.a[1], 1)
check('json.decode.a2', decoded.a[2], 2.5)
check('json.decode.a3', decoded.a[3], 's\n')
check('json.decode.c', decoded.c, false)
local deep = {}
for _ = 1, 300 do
    deep = {deep}
end
ok, err = pcall(json.encode, deep)
check('json.deep.ok', ok, false)
check('json.deep.msg', err:match('nesting too deep') ~= nil, true)
ok, err = pcall(json.decode, '[1, 2')
check('json.bad.ok', ok, false)
check('json.bad.msg', err:match("expected ']'") ~= nil, true)
local long = string.rep('y', 5000)
check('json.long', json.encode({long}), '["' .. long .. '"]')
for _ = 1, 200 do
    ok, err = pcall(json.encode, {long, deep})
    check('json.grown.ok', ok, false)
end
ok, err = pcall(json.encode, {[{}] = 1})
check('json.key.ok', ok, false)
check('json.key.msg', err:match('cannot encode table key') ~= nil, true)

local mxmod = require('mxmod')
check('mxmod.add', mxmod.add(4, 5), 9)
check('mxmod.greet', mxmod.greet('you'), 'hello, you')

local f, msg, where = package.loadlib(dir .. '/luamod.so', 'luaopen_missing')
check('loadlib.missing', f, nil)
check('loadlib.where', where, 'init')
f, msg, where = package.loadlib(dir .. '/nope.so', 'luaopen_nope')
check('loadlib.nofile', f, nil)
check('loadlib.nofile.where', where, 'open')
f = assert(package.loadlib(dir .. '/luamod.so', 'luaopen_luamod'))
check('loadlib.open', type(f()), 'table')
check('loadlib.star', package.loadlib(dir .. '/luamod.so', '*'), true)

ok, err = pcall(require, 'nosuchmod')
check('require.missing', ok, false)
check('require.missing.msg', err:match("no file '.-nosuchmod.so'") ~= nil, true)

local co = coroutine.wrap(function()
    for i = 1, 3 do
        coroutine.yield(luamod.add(i, i))
    end
    return pcall(luamod.boom)
end)
check('co.1', co(), 2)
check('co.2', co(), 4)
check('co.3', co(), 6)
local cok, cerr = co()
check('co.boom', cok, false)
check('co.boom.msg', cerr, 'boom from C')

local co2 = coroutine.wrap(function(n)
    local back = luamod.twice(n)
    return 'resumed with ' .. back
end)
check('yield.value', co2(21), 42)
check('yield.resume', co2('x'), 'resumed with x')

local a, b = luamod.call(function(x, y) return y, x end, 1, 2)
check('call.a', a, 2)
check('call.b', b, 1)
ok, err = pcall(luamod.call, function() error('inner', 0) end)
check('call.err.ok', ok, false)
check('call.err.msg', err, 'inner')
local status, value = luamod.try(function() error('caught', 0) end)
check('try.status', status, 2)
check('try.value', value, 'caught')
status, value = luamod.try(function() return 'fine' end)
check('try.ok', status, 0)
check('try.ok.value', value, 'fine')

local abi = require('luaabi')
check('abi.fns', abi.fns, true)
check('abi.gsub', abi.gsub('a.b.c', '.', '::'), 'a::b::c')
local got, status, after = abi.drive(function(...)
    local n = 0
    for k = 1, 3 do
        local back = coroutine.yield(k * 10, ...)
        n = n + back
    end
    collectgarbage()
    return 'done', n
end)
check('drive.status', status, 0)
check('drive.after', after, 0)
check('drive.values', table.concat(got, ','), '10,20,30,done,9')
got, status = abi.drive(function() coroutine.yield('x') error('drive failed', 0) end)
check('drive.err.status', status, 2)
check('drive.err.values', table.concat(got, ','), 'x,drive failed')
got, status = abi.drive(coroutine.yield)
check('drive.cbody', status, 0)
local suspended = coroutine.create(function() coroutine.yield() end)
coroutine.resume(suspended)
local was, closed = abi.peek(suspended)
check('peek.status', was, 1)
check('peek.closed', closed, 0)
check('peek.dead', coroutine.status(suspended), 'dead')
local retline
local counted, lines, lastline, same = abi.hooked(function()
    local s = 0
    for i = 1, 5000 do
        s = s + i
    end
    retline = debug.getinfo(1, 'l').currentline return s
end)
check('hook.count', counted > 10, true)
check('hook.lines', lines > 10, true)
check('hook.lastline', lastline, retline)
check('hook.get', same, true)
local filled, depth = abi.fill(5000)
check('abi.fill.len', #filled, 5001)
check('abi.fill.tail', filled:sub(-3), 'gh!')
check('abi.fill.top', depth, 2)
for _ = 1, 100 do
    ok, err = pcall(abi.fill, 3000, true)
end
check('abi.fill.err', err, 'failed after 3001 bytes')
check('abi.fstring', abi.fstring(), 'str|42|1099511627776|z|\u{20AC}|%')
local q, neg, lt = abi.arith(7, 2)
check('abi.idiv', q, 3)
check('abi.unm', neg, -7)
check('abi.lt', lt, false)
local what, line, src = abi.info() local here = debug.getinfo(1, 'l').currentline
check('abi.info.what', what, 'main')
check('abi.info.line', line, here)
check('abi.info.src', src:match('test%.lua$') ~= nil, true)
check('abi.globals', abi.globals(), true)
local st, chunk = abi.load('return 1 + 1')
check('abi.load', st, 0)
check('abi.load.run', chunk(), 2)
st, chunk = abi.load('return +')
check('abi.load.bad', st, 3)
check('abi.load.msg', type(chunk), 'string')

print('ok')
