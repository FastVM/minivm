#! /usr/bin/lua
--
-- lua-Harness : <https://fperrad.frama.io/lua-Harness/>
--
-- Copyright (C) 2009-2021, Perrad Francois
--
-- This code is licensed under the terms of the MIT/X11 license,
-- like Lua itself.
--

--[[

=head1 Lua Stand-alone

=head2 Synopsis

    % prove 241-standalone.t

=head2 Description

See section "Lua Stand-alone" in "Reference Manual"
L<https://www.lua.org/manual/5.1/manual.html#6>,
L<https://www.lua.org/manual/5.2/manual.html#7>,
L<https://www.lua.org/manual/5.3/manual.html#7>,
L<https://www.lua.org/manual/5.4/manual.html#7>

=cut

--]]

require'test_assertion'
local has_bytecode = not ravi
local has_error52 = _VERSION >= 'Lua 5.2'
local has_error53 = _VERSION >= 'Lua 5.3'
local has_opt_E = _VERSION >= 'Lua 5.2' or jit
local has_opt_W = _VERSION >= 'Lua 5.4'
local banner = '^[%w%s%-%.]-Copyright %(C%) %d%d%d%d'
if jit and jit.version:match'^RaptorJIT' then
    banner = '^[%w%s%.]- %-%- '
elseif ravi then
    banner = '^Ravi %d%.%d%.%d'
end

local lua = _retrieve_progname()
local luac = jit and lua or (lua .. 'c')
local exists_luac = io.open(luac, 'r')

if not pcall(io.popen, lua .. [[ -e "a=1"]]) then
    skip_all "io.popen not supported"
end

plan'no_plan'
diag(lua)

local f = io.open('hello-241.lua', 'w')
f:write([[
print 'Hello World'
]])
f:close()

local cmd = lua .. " hello-241.lua"
f = io.popen(cmd)
equals(f:read'*l', 'Hello World', "file")
f:close()

cmd = lua .. " -- hello-241.lua"
f = io.popen(cmd)
equals(f:read'*l', 'Hello World', "-- file")
f:close()

cmd = lua .. " no_file-241.lua 2>&1"
f = io.popen(cmd)
matches(f:read'*l', "^[^:]+: cannot open no_file%-241%.lua", "no file")
f:close()

if has_bytecode and exists_luac then
    if jit then
        os.execute(lua .. " -b hello-241.lua hello-241.luac")
    else
        os.execute(luac .. " -s -o hello-241.luac hello-241.lua")
    end
    cmd = lua .. " hello-241.luac"
    f = io.popen(cmd)
    equals(f:read'*l', 'Hello World', "bytecode")
    f:close()
    os.remove('hello-241.luac') -- clean up

    if not jit then
        os.execute(luac .. " -s -o hello-hello-241.luac hello-241.lua hello-241.lua")
        cmd = lua .. " hello-hello-241.luac"
        f = io.popen(cmd)
        equals(f:read'*l', 'Hello World', "combine 1")
        equals(f:read'*l', 'Hello World', "combine 2")
        f:close()
        os.remove('hello-hello-241.luac') -- clean up
    end
end

cmd = lua .. " < hello-241.lua"
f = io.popen(cmd)
if ravi then
    matches(f:read'*l', banner)
    matches(f:read'*l', '^Copyright %(C%)')
    matches(f:read'*l', '^Portions Copyright %(C%)')
    matches(f:read'*l', '^Options')
    equals(f:read'*l', '> Hello World', "redirect")
else
    equals(f:read'*l', 'Hello World', "redirect")
end
f:close()

cmd = lua .. " - < hello-241.lua"
f = io.popen(cmd)
equals(f:read'*l', 'Hello World', "redirect")
f:close()

cmd = lua .. " -i hello-241.lua < hello-241.lua 2>&1"
f = io.popen(cmd)
matches(f:read'*l', banner, "-i")
if ravi then
    matches(f:read'*l', '^Copyright %(C%)')
    matches(f:read'*l', '^Portions Copyright %(C%)')
    matches(f:read'*l', '^Options')
end
equals(f:read'*l', 'Hello World')
f:close()

cmd = lua .. [[ -e"a=1" -e "print(a)"]]
f = io.popen(cmd)
equals(f:read'*l', '1', "-e")
f:close()

cmd = lua .. [[ -e "error('msg')"  2>&1]]
f = io.popen(cmd)
equals(f:read'*l', lua .. [[: (command line):1: msg]], "error")
equals(f:read'*l', "stack traceback:", "backtrace")
f:close()

cmd = lua .. [[ -e "error(setmetatable({}, {__tostring=function() return 'MSG' end}))"  2>&1]]
f = io.popen(cmd)
if has_error52 or jit then
    equals(f:read'*l', lua .. [[: MSG]], "error with object")
else
    equals(f:read'*l', lua .. [[: (error object is not a string)]], "error with object")
end
if jit then
    equals(f:read'*l', "stack traceback:", "backtrace")
else
    equals(f:read'*l', nil, "not backtrace")
end
f:close()

cmd = lua .. [[ -e "error{}"  2>&1]]
f = io.popen(cmd)
if has_error53 then
    equals(f:read'l', lua .. [[: (error object is a table value)]], "error")
    equals(f:read'l', "stack traceback:", "backtrace")
elseif has_error52 then
    equals(f:read'*l', lua .. [[: (no error message)]], "error")
    equals(f:read'*l', nil, "not backtrace")
else
    equals(f:read'*l', lua .. [[: (error object is not a string)]], "error")
    equals(f:read'*l', nil, "not backtrace")
end
f:close()

cmd = lua .. [[ -e"a=1" -e "print(a)" hello-241.lua]]
f = io.popen(cmd)
equals(f:read'*l', '1', "-e & script")
equals(f:read'*l', 'Hello World')
f:close()

cmd = lua .. [[ -e"a=1" -i < hello-241.lua 2>&1]]
f = io.popen(cmd)
matches(f:read'*l', banner, "-e & -i")
f:close()

cmd = lua .. [[ -e "?syntax error?" 2>&1]]
f = io.popen(cmd)
matches(f:read'*l', "^.-%d: unexpected symbol near '%?'", "-e bad")
f:close()

cmd = lua .. [[ -e 2>&1]]
f = io.popen(cmd)
if _VERSION ~= 'Lua 5.1' then
    matches(f:read'*l', "^[^:]+: '%-e' needs argument", "-e w/o arg")
end
matches(f:read'*l', "^usage: ")
f:close()

cmd = lua .. [[ -v 2>&1]]
f = io.popen(cmd)
local copyright = f:read'*l'
matches(copyright, banner, "-v")
f:close()

cmd = lua .. [[ -v hello-241.lua 2>&1]]
f = io.popen(cmd)
matches(f:read'*l', banner, "-v & script")
if ravi then
    matches(f:read'*l', '^Copyright %(C%)')
    matches(f:read'*l', '^Portions Copyright %(C%)')
    matches(f:read'*l', '^Options')
end
equals(f:read'*l', 'Hello World')
f:close()

cmd = lua .. [[ -v -- 2>&1]]
f = io.popen(cmd)
matches(f:read'*l', banner, "-v --")
f:close()

if has_opt_E then
    cmd = lua .. [[ -E hello-241.lua 2>&1]]
    f = io.popen(cmd)
    equals(f:read'*l', 'Hello World', "-E")
    f:close()
else
    diag("no -E")
end

cmd = lua .. [[ -u 2>&1]]
f = io.popen(cmd)
if _VERSION ~= 'Lua 5.1' then
    matches(f:read'*l', "^[^:]+: unrecognized option '%-u'", "unknown option")
end
matches(f:read'*l', "^usage: ")
f:close()

cmd = lua .. [[ --u 2>&1]]
f = io.popen(cmd)
if _VERSION ~= 'Lua 5.1' then
    matches(f:read'*l', "^[^:]+: unrecognized option '%-%-u'", "unknown option")
end
matches(f:read'*l', "^usage: ")
f:close()

f = io.open('foo.lua', 'w')
f:write([[
function FOO () end
]])
f:close()

cmd = lua .. [[ -lfoo -e "print(type(FOO))"]]
f = io.popen(cmd)
equals(f:read'*l', 'function', "-lfoo")
f:close()

cmd = lua .. [[ -l foo -e "print(type(FOO))"]]
f = io.popen(cmd)
equals(f:read'*l', 'function', "-l foo")
f:close()

os.remove('foo.lua') -- clean up

if _VERSION >= 'Lua 5.4' and copyright >= 'Lua 5.4.4' then
    f = io.open('foo.lua', 'w')
    f:write([[
return function () end
]])
    f:close()

    cmd = lua .. [[ -lFOO=foo -e "print(type(FOO))"]]
    f = io.popen(cmd)
    equals(f:read'*l', 'function', "-lFOO=foo")
    f:close()

    cmd = lua .. [[ -l FOO=foo -e "print(type(FOO))"]]
    f = io.popen(cmd)
    equals(f:read'*l', 'function', "-l FOO=foo")
    f:close()

    os.remove('foo.lua') -- clean up
end

cmd = lua .. [[ -l lpeg -e "print(1)" 2>&1]]
f = io.popen(cmd)
not_equals(f:read'*l', nil, "-l lpeg")
f:close()

cmd = lua .. [[ -l no_lib hello-241.lua 2>&1]]
f = io.popen(cmd)
matches(f:read'*l', "^[^:]+: module 'no_lib' not found:", "-l no lib")
f:close()

if has_opt_W then
    cmd = lua .. [[ -W -e "warn'foo'" 2>&1]]
    f = io.popen(cmd)
    equals(f:read'*l', 'Lua warning: foo', "-W")
    equals(f:read'*l', nil)
    f:close()
else
    diag("no -W")
end

os.remove('hello-241.lua') -- clean up
done_testing()

-- Local Variables:
--   mode: lua
--   lua-indent-level: 4
--   fill-column: 100
-- End:
-- vim: ft=lua expandtab shiftwidth=4:
