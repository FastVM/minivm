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
- A local that is never read again doesn't keep its value alive, though it is still in scope. Its object can be collected, and finalized, before the block ends. If its `__gc` field is set only after that point, the finalizer never runs. testes gc.lua line 457 depends on this and fails when a collection lands between those lines, as it does under `MX_GC_STRESS`.
- `math.random` uses xoshiro256** seeded as reference Lua does, so `math.random()` and `math.random(0)` give the same values for the same seed. Ranges (`math.random(m, n)`) are projected differently, so their values differ.
- A `string.pack` result that runs past `MX_STD_MAX_STRING` (2 GiB) fails with `resulting string too large`, where reference Lua tries to allocate it and fails with `not enough memory`.

## Messages and introspection

- `string.dump` with `strip` drops local and upvalue names but keeps the chunk name and line information. A function loaded from it reports its real source and lines in errors, tracebacks and `debug.getinfo`, where reference Lua reports `?`, and its `activelines` isn't empty.
- `debug.getlocal` with a negative index returns `nil`, where reference Lua returns `(vararg)` and the vararg at that position.

## By design

- Binary chunks use MiniVM's own format, so they aren't interchangeable with reference Lua bytecode. `load` rejects truncated and structurally malformed chunks, but like reference Lua it doesn't prove a chunk type-safe, so don't load untrusted binary chunks.
- In incremental mode (`collectgarbage("incremental")`, `lua_gc(L, LUA_GCINC)`) marking runs in steps between which the program keeps running, but sweeping happens all at once at the end of each cycle, and objects are never moved to compact the heap. Full collections (`collectgarbage("collect")`) still compact.

## C API

C modules built against the Lua 5.5 headers load and run, and `make lib` builds `build/lib/libminivm.a` for embedding MiniVM in a host program. Errors raised from C (`lua_error`, `luaL_error`, `luaL_check*`) unwind with `longjmp`, as in reference Lua, and an error outside any protected call calls the `lua_atpanic` function and aborts. Threads, C debug hooks, warning functions, `lua_gc` and the host-side functions (`lua_newstate`, `luaL_newstate`, `lua_close`, `luaL_openselectedlibs`) work.

These parts differ:

- Continuations: the `k` of `lua_callk` and `lua_pcallk` is used only when the C function runs in a coroutine and the called function is a Lua function. In that case `k` always finishes the C function, with status `LUA_YIELD` even if nothing yielded, so code after the `lua_callk` doesn't run. Elsewhere `k` is ignored and the call returns normally, so a yield inside a C function called that way fails with `attempt to yield across a C-call boundary`. After an error, the continuation of `lua_pcallk` always gets `LUA_ERRRUN`.
- Custom allocators: `lua_newstate` and `lua_setallocf` record the allocator, and `lua_getallocf` returns it, but MiniVM allocates its own memory with `malloc` and never calls it. Running out of memory ends the process, so `lua_pcallk` never returns `LUA_ERRMEM`.
- `lua_newstate` ignores its `seed`. The standard libraries exist inside every state even when `luaL_openselectedlibs` leaves them out of the globals, so strings always have their methods.
- `lua_getextraspace`: a coroutine created from Lua gets its copy of the main thread's block when the C API first uses that coroutine, not when it is created.
- `lua_getinfo` with `r` always reports zero transferred values.
