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

local function probe(x)
    local y = x + 1
    return y
end
local nlines, pushed_f = abi.active(probe)
local want_lines = 0
for _ in pairs(debug.getinfo(probe, 'L').activelines) do
    want_lines = want_lines + 1
end
check('abi.lines.count', nlines, want_lines)
check('abi.lines.f', pushed_f, true)
local est, emsg = abi.errerr(function() error('first') end, function() error('again') end)
check('abi.errerr.status', est, 5)
check('abi.errerr.msg', emsg, 'error in error handling')
est, emsg = abi.errerr(function() error('first', 0) end, function(m) return 'handled ' .. m end)
check('abi.errrun.status', est, 2)
check('abi.errrun.msg', emsg, 'handled first')
local function joined(...)
    local parts = {}
    for i = 1, select('#', ...) do
        parts[i] = tostring((select(i, ...)))
    end
    return table.concat(parts, ',')
end
local kco = coroutine.wrap(function(...) return abi.kyield(...) end)
check('abi.kyield.first', kco('a', 'b'), 'b')
check('abi.kyield.resume', joined(kco('x', 'y')), 'a,x,y,1,42')
check('abi.kcall.main', joined(abi.kcall(function(a) return a + 1 end, 1)), 'below,2,0,7')
kco = coroutine.wrap(function()
    return abi.kcall(function(a)
        local b = coroutine.yield(a)
        return a, b
    end, 5)
end)
check('abi.kcall.yield', kco(), 5)
check('abi.kcall.resume', joined(kco(6)), 'below,5,6,1,7')
kco = coroutine.wrap(function()
    return abi.kpcall(function()
        coroutine.yield('w')
        error('late', 0)
    end)
end)
check('abi.kpcall.yield', kco(), 'w')
check('abi.kpcall.error', joined(kco()), 'below,late,2,9')
kco = coroutine.wrap(function()
    return abi.kpcall(function()
        coroutine.yield('v')
        return 1, 2, 3
    end)
end)
check('abi.kpcall.yield2', kco(), 'v')
check('abi.kpcall.ok', joined(kco()), 'below,1,1,9')
kco = coroutine.wrap(function()
    local ok, err = pcall(abi.kcall, function()
        coroutine.yield('z')
        error('through', 0)
    end)
    return ok, err
end)
check('abi.kcall.yield3', kco(), 'z')
check('abi.kcall.error', joined(kco()), 'false,through')
local same, gsubfn = abi.samefn()
check('abi.samefn', same, true)
check('abi.samefn.lib', gsubfn, abi.gsub)
local maxint, floored, limits, point = abi.conf()
check('abi.conf.int', maxint, tostring(math.maxinteger))
check('abi.conf.floor', floored, 2)
check('abi.conf.limits', limits, true)
check('abi.conf.point', point, '.')
local after_inc, after_gen, pause, before, after_set, collected = abi.gc()
check('abi.gc.inc', after_inc, 8)
check('abi.gc.gen', after_gen, 7)
check('abi.gc.param', before, pause)
check('abi.gc.param.set', after_set, pause + 50)
check('abi.gc.collect', collected, 0)
abi.extra(5)
local inner = coroutine.wrap(function() return abi.extra(7), abi.extra(8) end)
local first, second = inner()
check('abi.extra.copied', first, 5)
check('abi.extra.own', second, 7)
check('abi.extra.main', abi.extra(0), 5)

local pat = require('luapattern')
check('pat.quantize', pat.quantize(14.7, 5.0), 15.0)
check('pat.quantize.default', pat.quantize(14.7), 15.0)
check('pat.quantize.neg', pat.quantize(-3.2, 2.0), -4.0)

local g = pat.grid(10.0)
check('grid.count.empty', g:count(), 0)
g:insert(101, 5.0, 5.0)
g:insert(102, 12.0, 8.0)
g:insert(103, 50.0, 50.0)
check('grid.count', g:count(), 3)

local res = g:query(5.0, 5.0, 10.0)
table.sort(res)
check('grid.query.len', #res, 2)
check('grid.query.1', res[1], 101)
check('grid.query.2', res[2], 102)

local far = g:query(50.0, 50.0, 5.0)
check('grid.query.far.len', #far, 1)
check('grid.query.far.1', far[1], 103)

g:clear()
check('grid.count.cleared', g:count(), 0)

local calls = 0
local memo_sin = pat.memo(function(x)
    calls = calls + 1
    return math.sin(x)
end)
local s1 = memo_sin(1.5)
local s2 = memo_sin(1.5)
check('memo.val', s1, s2)
check('memo.calls', calls, 1)
local s3 = memo_sin(2.5)
check('memo.calls2', calls, 2)

-- Stem-and-leaf decomposition
local stems = pat.stemleaf({12, 15, 23, 27, 31}, 10)
check('stemleaf.1.len', #stems[1], 2)
check('stemleaf.1.1', stems[1][1], 2)
check('stemleaf.1.2', stems[1][2], 5)
check('stemleaf.2.len', #stems[2], 2)
check('stemleaf.2.1', stems[2][1], 3)
check('stemleaf.2.2', stems[2][2], 7)
check('stemleaf.3.len', #stems[3], 1)
check('stemleaf.3.1', stems[3][1], 1)

-- Pattern predictor
local pred, conf, kind = pat.predict({2, 4, 6, 8, 10})
check('predict.linear.pred', pred, 12)
check('predict.linear.kind', kind, 'linear')
check('predict.linear.conf', conf, 1.0)

pred, conf, kind = pat.predict({3, 9, 27, 81})
check('predict.geom.pred', pred, 243)
check('predict.geom.kind', kind, 'geometric')

pred, conf, kind = pat.predict({42, 42, 42, 42})
check('predict.const.pred', pred, 42)
check('predict.const.kind', kind, 'constant')

pred, conf, kind = pat.predict({1, 2, 3, 1, 2, 3, 1, 2})
check('predict.cycle.pred', pred, 3)
check('predict.cycle.kind', kind, 'cycle')

-- Exact distance query on spatial grid
g:insert(101, 5.0, 5.0)
g:insert(102, 12.0, 8.0)
local exact_res = g:query(5.0, 5.0, 5.0, true)
check('grid.query.exact.len', #exact_res, 1)
check('grid.query.exact.1', exact_res[1], 101)

-- Spectral Walsh-Hadamard Transform
local spec = pat.wht({1, 1, 1, -1})
check('wht.len', #spec, 4)
check('wht.0', spec[1], 2)
check('wht.1', spec[2], 2)
check('wht.2', spec[3], 2)
check('wht.3', spec[4], -2)

-- Dozenal Phase Exact Invariant
check('dozenal.third', pat.dozenal(2 * math.pi / 3), 1024)
check('dozenal.half', pat.dozenal(math.pi), 1536)
check('dozenal.quarter', pat.dozenal(math.pi / 2), 768)

-- Chebyshev Spectral Function Compression
local c_sin = pat.chebyshev(math.sin, -1.0, 1.0, 5)
check('cheb.sin.0', math.abs(c_sin(0.0)) < 1e-6, true)
check('cheb.sin.half', math.abs(c_sin(0.5) - math.sin(0.5)) < 1e-4, true)

-- Discrete Fourier Phase Detector
local sig = {}
for i = 1, 64 do
    sig[i] = math.sin((i - 1) * 2 * math.pi / 8)
end
local freq, pconf = pat.phase(sig)
check('phase.freq', math.abs(freq - 0.125) < 1e-4, true)
check('phase.conf', pconf > 0.90, true)

print('ok')
