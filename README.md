![The MiniVM logo, a blueish grey brick (or maybe a can of tuna if you are a cat)](res/MiniVM.svg)

# MiniVM

MiniVM is an interpreter for Lua 5.5, written in C11. It has no JIT and needs nothing beyond libc, libm and libdl, so it runs on platforms that forbid generating machine code at runtime. This is version 0.1.0.

It specializes code on the types it actually sees, using lazy basic block versioning (the technique behind Ruby's YJIT), and runs the specialized code as direct-threaded code. On the benchmarks below it is 1.9x faster than reference Lua 5.5.1 on small programs and 1.2x faster on real ones.

## Performance

Each number compares MiniVM's time with another interpreter's, as a geometric mean over the benchmarks. Memory is peak RSS, compared the same way.

| Compared with | Micro-benchmarks | Real programs | All | Peak memory |
| --- | --- | --- | --- | --- |
| Lua 5.5.1 | 1.9x faster | 1.2x faster | 1.7x faster | 1.6x as much |
| LuaJIT 2.1 interpreter (`-joff`) | 1.2x faster | 1.3x slower | 1.1x faster | 1.5x as much |
| LuaJIT 2.1 with its JIT | 2.8x slower | 1.3x slower | 2.5x slower | 1.4x as much |

There are 29 micro-benchmarks and 7 real programs: the fennel compiler, the tl (Teal) type checker, json.lua, lulpeg, serpent, inspect and etlua. LuaJIT runs 28 and 4 of them. inspect, lulpeg and serpent use Lua 5.3 syntax, and `deep-rec` recurses deeper than LuaJIT's stack allows.

`strcat-accum` builds a string with `s = s .. x` in a loop, which MiniVM does with ropes in about 1% of the time. Without it, MiniVM is 1.7x faster than Lua 5.5.1 on micro-benchmarks, as fast as LuaJIT's interpreter, and 3.5x slower than LuaJIT's JIT.

tl is the one program where MiniVM is slower than Lua 5.5.1.

Times in milliseconds, best of 5 runs:

| Benchmark | MiniVM | Lua 5.5.1 | LuaJIT `-joff` | LuaJIT |
| --- | ---: | ---: | ---: | ---: |
| `branchy` | 344 | 546 | 668 | 324 |
| `calls-callable` | 341 | 319 | 239 | 30 |
| `calls-mono` | 390 | 784 | 395 | 46 |
| `calls-poly` | 157 | 284 | 233 | 35 |
| `closure-churn` | 187 | 409 | 325 | 284 |
| `concat` | 162 | 334 | 251 | 76 |
| `coro-pingpong` | 471 | 501 | 168 | 208 |
| `deep-rec` | 355 | 395 | - | - |
| `fannkuch` | 101 | 111 | 86 | 20 |
| `gc-churn` | 198 | 283 | 178 | 15 |
| `gc-strings` | 178 | 369 | 243 | 168 |
| `gc-survivors` | 187 | 334 | 122 | 113 |
| `list-deep` | 412 | 414 | 374 | 374 |
| `loop-math` | 491 | 1221 | 799 | 86 |
| `mandel` | 96 | 243 | 168 | 20 |
| `mixed-arith` | 233 | 406 | 197 | 71 |
| `nbody` | 1356 | 1760 | 941 | 111 |
| `poly-return` | 162 | 283 | 137 | 30 |
| `rec-mutual` | 416 | 588 | 398 | 40 |
| `spectralnorm` | 264 | 319 | 207 | 10 |
| `strbuild` | 51 | 76 | 61 | 51 |
| `strcat-accum` | 10 | 933 | 655 | 1731 |
| `table-churn` | 354 | 755 | 577 | 461 |
| `table-hash` | 106 | 248 | 137 | 30 |
| `table-oldmut` | 132 | 229 | 162 | 15 |
| `table-pairs` | 158 | 339 | 66 | 61 |
| `table-rw` | 86 | 148 | 116 | 15 |
| `trees-large` | 13174 | 50996 | 14236 | 12749 |
| `trees-small` | 130 | 153 | 76 | 56 |
| etlua | 193 | 274 | 167 | 132 |
| fennel | 223 | 234 | 172 | 168 |
| inspect | 1655 | 2072 | - | - |
| json.lua | 2356 | 2949 | 2057 | 1083 |
| lulpeg | 345 | 390 | - | - |
| serpent | 967 | 1300 | - | - |
| tl | 97 | 91 | 66 | 147 |

Each comparison ran MiniVM and the other interpreter interleaved, pinned to one core with `taskset`. The MiniVM column is from the run against Lua 5.5.1. The machine had other work running during these runs, so single rows are less reliable than the geometric means: MiniVM's own time for some benchmarks differed by up to 47% between the three runs (`calls-callable`, for example, took 341, 254 and 258 ms), while the micro-benchmark geometric mean against Lua 5.5.1 came out at 0.52x in all four full runs made while preparing this release. The harness checks for exit every 5 ms, so times under about 50 ms are only accurate to that.

The machine is an AMD Ryzen 9 7950X3D running Linux (KDE neon 24.04, based on Ubuntu 24.04), with GCC 13.3. Lua 5.5.1 is built with its own makefile (`-O2`), LuaJIT 2.1.1792588164 with its default `make`, and MiniVM with its default `make` (`-O3`). The benchmarks are in `test/bench/`, the real programs in `test/bench/real/`. To rerun a comparison, with CPython 3.12 or later:

```sh
make -C lua -s -j lua
BENCH_REPS=5 taskset -c 4 python3 test/bench/run.py --vs lua/lua
```

`--vs` takes any interpreter command, such as `--vs "luajit -joff"`. `--check` compares each benchmark's output with the other interpreter's instead of timing it.

## Building

```sh
git clone https://github.com/FastVM/minivm
cd minivm
make -j
```

This builds `build/bin/minivm`. It needs GCC or Clang, since the interpreter uses computed goto, and GNU Make. The REPL uses [isocline](https://github.com/daanx/isocline), which is included in `vendor/isocline`.

`make install` copies the binary to `$(PREFIX)/bin` and the C API headers to `$(PREFIX)/include/minivm`, so C modules build with `-I$(PREFIX)/include/minivm`. `PREFIX` defaults to `/usr/local`, and `DESTDIR` is prepended to both for staged installs.

## Usage

`minivm script.lua a b` runs `script.lua` with `a` and `b` in `arg`. With no script and none of `-e` or `-v`, MiniVM starts the REPL when standard input is a terminal, and otherwise runs standard input as a script.

The options are those of the reference `lua` command, plus `--ext`:

| Option | Meaning |
| --- | --- |
| `-e chunk` | Run the Lua text `chunk` |
| `-l mod` | `require` `mod` and store it in the global `mod` |
| `-l g=mod` | `require` `mod` and store it in the global `g` |
| `-i` | Start the REPL after everything else |
| `-v` | Print the version |
| `-W` | Turn warnings on |
| `-E` | Ignore `LUA_INIT`, `LUA_PATH` and `LUA_CPATH` |
| `--ext` | Enable MiniVM's extensions |
| `--` | Treat the next argument as the script |
| `-` | Run standard input as the script |

`LUA_INIT`, `LUA_PATH` and `LUA_CPATH`, and their `_5_5` forms, work as they do for `lua`. `require` loads Lua files and C modules built against the Lua 5.5 headers; `include/` has ABI-compatible versions of `lua.h`, `lauxlib.h`, `lualib.h` and `luaconf.h`.

## Extensions

`--ext` turns these on. They may change in later versions.

- `if` and `do` as expressions: `local x = if n > 0 then 'pos' else 'neg' end` and `local y = do local t = f(); t * 2 end`. An `if` with no matching branch is `nil`, and a `do` block's value is its last expression.
- `callcc(f)` calls `f(k)`, and calling `k(...)` makes `callcc` return `...` at once. Continuations are one-shot and escape-only: they work until `callcc` returns, and can't cross a C call. `callcci` is the same but accepts only functions, where `callcc` also accepts callable tables.
- `table.keys(t)` and `table.values(t)` return arrays of a table's keys or values.
- `unpack` is a global, as in Lua 5.1.
- `vm.stats()` returns a table of `versions` (the number of type-specialized block versions), `version_bytes` (the memory they use) and `heap_bytes`. `vm.gc()` forces a full collection, `vm.typename(v)` returns MiniVM's internal type name, `vm.version()` the version string, and `vm.setop(t, name, f)`, given a table `t`, replaces the handler of the operation `name` for every table.

## Compatibility

MiniVM implements Lua 5.5, including `global` declarations, named varargs, `<const>` and `<close>`, coroutines, metatables, weak tables, and the full standard library with patterns, `string.pack` and `string.dump`.

It passes 2180 of the 2183 [lua-Harness](https://fperrad.frama.io/lua-Harness/) tests that apply to Lua 5.5. Two of the other three expect binary chunks to start with the reference `\x1bLua` header, and one depends on the timezone and fails under reference Lua as well. Of the 30 files of the official Lua 5.5.1 test suite, run one at a time, 22 pass. Each file stops at its first difference, and [COMPATIBILITY.md](COMPATIBILITY.md) lists each one along with every other known difference.

## Limitations

- There is no JIT.
- Major garbage collections stop the program, so a large live heap means long pauses. `collectgarbage()` at a convenient moment, such as a loading screen, moves that pause there.
- MiniVM uses more memory than reference Lua: 1.6x the peak RSS on the benchmarks above, and 2.4x on the real programs, mostly for type-specialized code.
- MiniVM can't be embedded yet. C modules work, but the host side of the C API, creating a state with `lua_newstate` or `luaL_newstate`, isn't implemented.
- When an allocation sized by the script fails (`string.rep`, `table.create`, `table.concat`, string buffers, `io.read`), MiniVM raises the catchable error `not enough memory`, as reference Lua does. Running out of memory anywhere else ends the process with `minivm: not enough memory`.
- It is tested on x86-64 Linux with GCC and Clang. Other architectures and macOS are untested, and MSVC and Windows aren't supported.

## How it works

The lexer and parser (`src/lua/front`) turn source into MiniVM's IR (`src/ir`), a graph of basic blocks. When a block runs, it is specialized on the types of the values it is entered with (`src/version`), and that version is lowered to direct-threaded code, which the interpreter (`src/run`) executes. An operation whose result type isn't known until it runs, such as a table read or a call, ends a version, and the next version is chosen once the type is known.

Values are in `src/value`. The garbage collector (`src/vm/gc.c`) is generational, with an Immix-style heap of blocks and lines, and it moves tables and closures out of sparse blocks during major collections. The standard library and the C API are in `src/lua/rt`.

## Reporting bugs

Open an issue with the smallest script that shows the problem and the output of `minivm -v`. Crashes are always bugs.

## License

MIT. See [LICENSE](LICENSE). isocline, in `vendor/isocline`, is MIT-licensed separately; see [its license](vendor/isocline/LICENSE).
