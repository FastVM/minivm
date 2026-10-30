# Compatibility

MiniVM implements Lua 5.5. This file lists every known way it differs from the reference implementation, Lua 5.5.1.

## Test suites

[lua-Harness](https://fperrad.frama.io/lua-Harness/): 2180 of the 2183 tests that apply to Lua 5.5 pass. Two of the other three expect binary chunks to start with the reference `\x1bLua` header, and one depends on the timezone and fails under reference Lua as well.

The official test suite (`testes/` in the Lua 5.5.1 sources), run one file at a time: api, attrib, bitwise, closure, code, constructs, coroutine, cstack, events, gc, gengc, goto, literals, math, memerr, nextvar, pm, sort, strings, tpack, utf8 and verybig pass. big, files and main need the setup that `all.lua` does, and fail on their own under reference Lua too. Each of the other files stops at its first failed check, so this shows only the first difference in each:

| File | First difference |
| --- | --- |
| calls | binary chunks don't start with the reference `\x1bLua` header (line 541) |
| db | `activelines` of a function loaded from a stripped `string.dump` (line 57) |
| errors | error locations in a function loaded from a stripped `string.dump` (line 309) |
| locals | a `__close` handler that yields while an error is unwinding (line 1015) |
| vararg | reading a named vararg table allocates (line 249) |

To rerun it, build Lua 5.5.1 in `lua/` and its test modules in `lua/testes/libs`, then run `python3 test/testes/run.py --vs lua/lua`. `python3 test/compat/run.py --lua lua/lua testes_gaps.lua` reproduces most of the differences below in a few lines each.

## Behavior

- Hooks are set per thread, but every thread shares one hook function, mask and count. `debug.sethook(co, f, mask)` hooks only `co`, and also replaces the function and mask of any other thread that has a hook.
- A `__close` handler run while an error is unwinding can't yield; the yield fails with `attempt to yield across a C-call boundary`. Handlers run on a normal scope exit or a `return` can.
- A `__pairs` metamethod can yield when `pairs` is called for at most 4 results, as in a generic `for`. When all of its results are kept, as in `select('#', pairs(t))`, the yield fails with `attempt to yield across a C-call boundary`.
- A named vararg parameter (`...t`) is a table built when the function is entered, so reading it allocates.
- `math.random` uses xoshiro256** seeded as reference Lua does, so `math.random()` and `math.random(0)` give the same values for the same seed. Ranges (`math.random(m, n)`) are projected differently, so their values differ.
- A `string.pack` result that runs past `MX_STD_MAX_STRING` (2 GiB) fails with `resulting string too large`, where reference Lua tries to allocate it and fails with `not enough memory`.

## Messages and introspection

- `string.dump` with `strip` drops local and upvalue names but keeps the chunk name and line information. A function loaded from it reports its real source and lines in errors, tracebacks and `debug.getinfo`, where reference Lua reports `?`, and its `activelines` isn't empty.
- `debug.getlocal` with a negative index returns `nil`, where reference Lua returns `(vararg)` and the vararg at that position.

## By design

- Binary chunks use MiniVM's own format, so they aren't interchangeable with reference Lua bytecode. `load` rejects truncated and structurally malformed chunks, but like reference Lua it doesn't prove a chunk type-safe, so don't load untrusted binary chunks.
- `collectgarbage("incremental")` switches to non-generational collection. It doesn't do incremental marking.

## C API

C modules built against the Lua 5.5 headers load and run. Errors raised from C (`lua_error`, `luaL_error`, `luaL_check*`) unwind with `longjmp`, as in reference Lua. Threads (`lua_newthread`, `lua_resume`, `lua_tothread`, `lua_closethread`), C debug hooks (`lua_sethook`), warning functions (`lua_setwarnf`, `lua_warning`) and `lua_yield` from a C function work.

These parts don't:

- Continuation functions: `lua_yieldk` with a `k`, and the `k` of `lua_callk` and `lua_pcallk`, which is ignored.
- Custom allocators: `lua_setallocf` does nothing, and `lua_getallocf` returns the default allocator.
- `lua_getextraspace` is one block shared by every C call, not one per thread.
- `lua_pcallk` returns `LUA_ERRRUN` for every error, never `LUA_ERRMEM` or `LUA_ERRERR`.
- `lua_gc`: `LUA_GCCOLLECT` and `LUA_GCSTEP` only schedule a collection, which runs at the next safepoint rather than before `lua_gc` returns. `LUA_GCGEN` and `LUA_GCINC` always return `LUA_GCGEN`, and `LUA_GCPARAM` returns 0.
- `lua_getinfo`: `L` pushes `nil` instead of a table of lines, `t` always reports a non-tail call, and `r` always reports zero transferred values.
- `lua_atpanic` does nothing and returns `NULL`.
- `luaconf.h` defines only the macros the rest of the headers need, so modules that use others, such as `LUA_MAXUNSIGNED`, `l_mathop` or `lua_getlocaledecpoint`, don't compile.
- The host side: `lua_newstate` and `luaL_newstate` return `NULL`, and `lua_close` and `luaL_openselectedlibs` do nothing, so MiniVM can't be embedded yet.
