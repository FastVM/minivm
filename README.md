![The MiniVM Logo, looks like a brick according to some, some catfood according to others](MiniVM.svg)

# MiniVM

[![Link to Discord](https://img.shields.io/discord/814855814514737152?logo=discord&color=5865F2)](https://discord.gg/UyvxuC5W5q)

MiniVM is a small and fast cross-language VM written in good ol' C, meaning it can compile and run just about anywhere.

Here are a few reasons why MiniVM is pretty neat:

- Built on a register-based ISA that beats luajit—*with the JIT on*—in some benchmarks. (See the benchmark section below).
- Has a 3-buffer *pauseless* GC for latency-sensitive applications.
- Supports a flexible data model, with NaN-boxing and pointer compaction, for efficient memory usage.
- Leverages Cosmopolitan libc + WebAssembly for easy crossplatform portability.
- ... check out the details section for more!

MiniVM is small and flexible enough to run just about any language under the sun (given you've taken the time to write a compiler for it). Front ends we've experimented with include Lua, Scheme, Paka, and others.

## History
This project started as an exploration into what it takes to build a fast interpreters. The first version was blocked out during a single Discord call, and ran a small lisp-like language.

This original implementation was a plain stack machine, which, for whatever reason, was a tad faster than it should've been.

Leveraging this tiny 1,000 LoC base, MiniVM matured into something a bit bigger, but only slightly. It now runs a language close to ASM, and has gotten quite a lot faster over time.

MiniVM's speed is in no small part due to its architecture. It's a register/stack machine with carefully-selected opcodes that are designed to work well with common data-access patterns.

Above all else, MiniVM is a constantly improving each day. We hope you find the journey to be as interesting as the final destination. If you're interested as to where the project is headed next, ping Shaw (`@4984#4984`) on [the Discord Server](https://discord.gg/UyvxuC5W5q).

## Some Sweet Deets
Below is a small discussion of the architecture of MiniVM, and the emergent properties because of these decisions.

### A Register-Based VM
First and foremost, MiniVM is a register-based VM. This means that instructions are larger and operate on *registers*, as opposed to the *stack*. Because of this property, register-based VMs play nicely with modern hardware, and result in less instructions per unit of work done. Consider the following:

```
while x < 1000 {
  x = x + 1
}
```

A stack-based VM would have to emit a single instruction for every single little thing done in the loop above. You'd probably end up with something like:

```
head_of_loop:
  load_var x
  push_int 1000
  less_than
  jump_if_false :end_of_loop
  load_var x
  increment
  store_var x
end_of_loop:
```

This is all well and good, but compare it to what a register-based machine does:

```
// r0 is where x lives
head_of_loop:
  jump_if_reg_less_than_number r0 1000 :end_of_loop
  increment_reg r0 1
  jump :head_of_loop
end_of_loop:
```

Although each instruction is a bit more complex, there are way fewer instructions. And this per-instruction complexity isn't necessarily a bad thing: making instructions more complex offloads work to the host language (C, in this case), which means that the runtime can compile common complex instructions to efficient native code.

### Wasm and Portability
MiniVM itself is a small pure-C project with few dependencies; for this reason it's pretty easy to compile to just about any target.

This, of course, means that the MiniVM runtime can be compiled to Wasm: the resulting binary is only *12KiB* in size! This is small enough to embed in a website, if that's your cup of tea.

### On `malloc` and `putchar`

> minivm performs better if pthreads are used but only really needs putchar.
> malloc is required for memory allocations as of recent.
>
> — 4984

TODO: Expand this section.

### Simple Type Model
MiniVM implements 3 heavily-optimized core types:

1. numbers (classic IEEE-754 doubles)
2. general-purpose arrays
3. function pointers

From these core types, common types are derived, including tuples, unions and tagged unions (enums), and closures.

Each of the 3 core types heavily optimized through the use of NaN-tagging (cramming a 48-bit pointer into the mantissa of a NaN), and pauseless-gc-enabled pointer compaction.

Because MiniVM is a register-based machine, it employs clever instructions to leverage common type layouts for better performance. For instance, to emulate closures arrays can be called as functions if the first item in that array is a function.

## Installation
Building from source is pretty simple:

```
git clone https://github.com/shawsumma/minivm
cd minivm
make
```

The compiled binary will be in the `bin` folder. If you'd like to build using Wasm, instead of `make`, run the following:

```
make -f wasm.mak
```

You may need to install a few dependencies:

- `putchar`
- `pthread` (optional)
- `libc` (if not using cosmo)
- `fmod` (optional)

Most OSes should have these core libraries out of the box. Other than that, `pthreads` is only needed if you want to run MiniVM on bare metal.

## Getting Started
TODO(4984): Document API and maybe a short getting-started example.

## Benchmarks
TODO(4984): Attach some benchmarks...

## Roadmap
TODO(4989): Features you're planning to tackle next...

> Note: MiniVM is wholly developed by Shaw (4984); this README was written by a friend of his who thinks he can be a bit too modest at times.
