# MiniVM
A Small VM.

MiniVM is a virtual machine implemented in c. This peoject was originally an exploration into fast interpreters.

Originally a stack machine, MiniVM was created to run a small lisp-like language, but now runs a language close to ASM.

MiniVM allows for aribtrary recursion, as well as tail calls. It is a register + stack machine with shorthand opcocdes.

## Building

With this repository cloned, type `make`.

If the command worked, you will have a minivm built in `./bin/minivm`

If make cannot find `gcc` command, try something like
`make CC=clang`
`make CC=tcc`

## Example

Make a file named `count_primes.asm` and put the following code in it.

```
    mov r0 1000000
    mov r5 1
    mov r1 1
    j [sum.body]
[sum.redo]
    add r1 2
[sum.body]
    mov r3 3
[mods.redo]
    mod r2 r1 r3
    jeq [sum.check] r2 0
    mul r2 r3 r3
    add r3 2
    jle [mods.redo] r2 r1
    add r5 1
[sum.check]
    jlt [sum.redo] r1 r0
    println r5
```

To assemble it: `./bin/minivm count_primes.asm -o count_primes.bc`
To run that asm: `./bin/minivm count_primes.bc`
To dis that asm: `./bin/minivm count_primes.bc -o count_primes_dis.asm`
To run that asm: `./bin/minivm count_primes_dis.asm`
