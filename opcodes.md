# MiniVM Opcode Format

Instructions are an [Opcode](#Opcodes) followed by some number of [Arguments](#Arguments)

# Arguments

Arguments are attatched to the last opcode.

## Integer

A 32 bit little endian unsigned number.

## Register

A local value.

Saved by the [call](#call) and [dcall](#dcall) instructions.

There are an arbitrary number of registers given to MiniVM, atleast 256 but no more than 4294967296. 

## Location

A reference to a piece of code.

# Opcodes

Opcodes are the base operations of MiniVM.

## exit

`exit` exits the current vm. 

It does *not* call `exit()` from the host process; The host sees exit as a return from `vm_run_arch_int`.

Stored as `0`

## reg

`reg` moves the content of rY into rX.

Stored as `1 X Y`

## int

`int` moves the value Y into rX

Stored as `2 X Y`

## jump

`jump` branches unconditionally to the location .X

Stored as `3 X`

## func

`func` branches unconditionally to the location @Y

Func performes the same function as [jump](#jump) when executed.

It is expected that [call](#call) and [dcall](#dcall) jump to a location directly after the end of a `func` instruction.

When calling a function an integer is expected to tell the callee how many registers to allocatate.

Stored as `4 X N`

## add

`add` moves the result of `rY + rZ` into rX

Stored as `5 X Y Z`

## sub

`sub` moves the result of `rY - rZ` into rX

Stored as `6 X Y Z`

## mul

`sub` moves the result of `rY * rZ` into rX

Stored as `7 X Y Z`

## div

`div` moves the result of `rY / rZ` into rX

Stored as `8 X Y Z`

## mod

`mod` moves the result of `rY % rZ` into rX

Stored as `9 X Y Z`

## call

`call` calls a subroutine.

N registers are loaded and moved to the new r1 to rN.

.Y is jumped to

All registers are restored after the call. 
The result of the `ret` from within the call is moved into rX

Stored as `10 X Y N` followd by N registers

### Examples
`10 0 4984 3 5 1 9` means `r0 <- call 4984(r5 r1 r9)`
`10 7 6 0` means `r7 <- call 6()`

## ret

`ret` returns to the last call instruction with value rY put into the caller's rX.

Stored as `11 Y`

## putchar

`putchar` prints a single character at unicode point rX.

Stored as `12 X`

## bb

`bb` branches conditionally on a boolean variable
- to .Y if rX == 0
- to .Z if rX != 0

Stored as `13 X Y Z`

## beq

`beq` branches conditionally on an equality
- to .Z if rX != rY
- to .W if rX == rY

Stored as `14 X Y Z W`

## blt

`blt` branches conditionally on a less than comparrason
- to .Z if rX >= rY
- to .W if rX < rY

Stored as `15 X Y Z W`

## dcall

`dcall` calls a subroutine that has address stored in rY.

N registers are loaded and moved to the new r1 .. rN.

the address in rY is jumped to.

All registers are restored after the call. 
The result of the `ret` from within the call is moved into rX

Stored as `16 X Y N` followd by N registers

## intf

`reg` moves the address of .Y into rX.

Stored as `17 X Y`

## tcall

`tcall` jumps to the head of the current routine with new arguments.

N registers are loaded and moved to r1..rN

Stored as `18 X N` followed by N registers

## pair

`pair` joins rY and rZ into a pair into rX.

The joined pair is freed automatically.

Stored as `19 X Y Z`

## first

`first` takes the first joined value in rY into rX.

Stored as `20 X Y`

## second

`first` takes the second joined value in rY into rX.

Stored as `21 X Y`

