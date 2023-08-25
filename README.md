![The MiniVM Logo, a blueish grey brick](res/MiniVM.svg)

# MiniVM

**MiniVM is a small and portable language virtual machine (VM) written in C**, meaning it can compile and run just about anywhere. 

Some reasons to use MiniVM are:

- MiniVM is *fast*
    - LuaJIT's interpreter, [known for its speed](http://lambda-the-ultimate.org/node/3851#comment-57761), takes nearly twice as long as MiniVM to execute similar code.
    - Types are gone at runtime. Data has no type associated with it.
        - Sanitization could be used to catch type errors.
- MiniVM is *small*
    - 34KiB when building with `make -B OPT='-O2 -fno-ssa-phiopt -s -fuse-ld=lld -Wl,--gc-sections' CC=gcc-11`
    - Single binary to assemble and run.
- MiniVM is portable
    - MiniVM can compile with `gcc`, `clang`, `tcc` and many more

## History
MiniVM started its life as an example of how to write a VM. It all started in a discord call, when the question of "how do i write a virtual machine" came up. Shaw wrote the first prototye in a couple hours and sent it on github as an example.

Development went along for several months, with many frontends being written, most notably, [Paka](https://github.com/fastvm/paka). Paka was originally written in Dlang, but was converted to be self hosted on minivm soon after. 

Then came the Hacker News [Post](https://news.ycombinator.com/item?id=29850562)... MiniVM was #1 on hacker news for a few hours. 

MiniVM has gone through many stages.
- Stack Based Prototype
- Conversion to Registers Machine
- Pauseless GC based on Ring Buffers
    - Loading a value, but pauses were gone
- Removal of arrays, in favor of the Cons Cell
- Floating point operations
- Assembler added

## Benchmarks
TODO: add benchmarks
TODO: automatic benchmarks

## Roadmap
- Continue Development of MiniVM
- Reduce dependancy to LibC
- Better Validators
- Continue to improve performance
