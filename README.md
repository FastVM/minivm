![The MiniVM Logo, a blueish grey brick (Or maybe can of tuna if you are a cat)](res/MiniVM.svg)

# MiniVM

MiniVM is a Small but Optimizing Virtual Machine and Runtime.

It has a just-in-time compiler based on [Cuik](https://github.com/realnegate/cuik)'s [TB](https://github.com/RealNeGate/Cuik/tree/master/tb).

Currently it supports Linux x86-64, FreeBSD amd64 with work going on to re-add Windows x64 support.

MiniVM is written in C11 with (minor GNU extensions), and builds with GCC and Clang, with TCC support not hard to patch in.

## Building

MiniVM uses GNU Make as it's build system.

### Requirements

You'll need
* The MiniVM repo
    * Make sure to get the cuik submodule
        * You can use `git clone github.com/FastVM/minivm --recursive`
        * If you've already cloned you can use `git submodule update --init`
* A C Compiler
    * GCC works
    * Clang works if you replace CC=gcc with CC=clang

### Build Configs

Here's some Shell Commands to build MiniVM different ways

* Debug - `make -Bj OPT='-g'`
* For Size - `make -Bj CC=gcc OPT='-s -Oz -flto -fno-asynchronous-unwind-tables -fomit-frame-pointer'`
    * GCC does a better job than Clang to make tiny binaries of MiniVM.
* For Speed - `make -Bj OPT='-O3 -flto'`

## Binary Size

* 142.9 KiB when built for size
* 284.1 KiB when built for speed
* 901.0 Kib when built for debug

## Speed

Here's some benchmark runs, they aren't too representative yet.

### Math + Recursion

```sh
shell> ./build/bin/minivm time run test/fib40.paka
102334155

--- took 1.284161s ---
```

```
shell> ./build/bin/minivm time run test/fib35.paka
9227465

--- took 0.113154s ---
```

### Startup Perf

```sh
shell> ./build/bin/minivm time eval 'env.io.debug("hello world")'
"hello world"

--- took 0.000544s ---
```
