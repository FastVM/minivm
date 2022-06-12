# MiniVM opcodes

## Exit

Return to caller, cleanup GC

```
exit
```

## Register Move

Move contents of `rY` to `rX`

```
rX <- reg rY
```


## Dynamic Jump

Jump to address stored in `rX`

```
djump rX
```

## Func

Jump over function, define `label.a`

```
func func.a
end
```

## Label Jump

Jump to `label.a`

```
jump label.a
```

## Label Call

Jump to `label.a`
Push next instruction address to internal stack
Arguments in `rA` is moved to `r1` at `label.a`, `rB` is stored to `r2`, `rC` -> `r3` and so on
Once the function is done all registers are restored.
The return value is put into `rX`


```
rX <- call label.a [rA? rB? rC...]
```

## Label Address

Store address of `label.a` to `rX`

```
rX <- addr label.a
```

## Dynamic Jump

Jump to address in `rX` previously obtained from [`Label Address`](#label-address)

```
djump rX
```

## Dynamic Call

```
rX <- dcall rY [rA? rB? rC...]
```

## Return

Store the `rY` into `rX` from [`Label Call`](#label-call) or [`Dynamic Call`](#dynamic-call).

```
ret rY
```

## Integer

Store `N` into `rX`

```
rX <- int N
```

## Add / Sub / Mul / Div / Mod

Store result of math operation `rY` op `rZ` into rX

```
rX <- add rY rZ
rX <- sub rY rZ
rX <- mul rY rZ
rX <- div rY rZ
rX <- mod rY rZ
```

## Branch Boolean

Jump to `label.a` if the contents of `rX` is zero, otherwise jump to `label.b`

```
bb rX label.a label.b
```

## Branch Equal

Jump to `label.t` if the contexnts of `rX` is equal to the contents of `rY` otherwise jump to `label.f`

```
beq rX rY label.f label.t
```

## Branch Less Than

Jump to `label.t` if the contexnts of `rX` is less than the contents of `rY` otherwise jump to `label.f`

```
blt rX rY label.f label.t
```

## String

Store an array with the ascii data representing "text-1" ints into `rX`

```
rX <- str :text-1
```

## Array

Store an empty array of length `rY` into `rX`

```
rX <- arr rY
```

## Set Array Index

Store `rZ` into `rX` at index `rY`

```
set rX rY rZ
```

## Get Array Index

Store into `rX` the element at index `rZ` of `rY`

```
rX <- get rY rZ
```

## Array Length

Store into `rX` the length of the array in `rY`

```
rX <- len rY
```

## Object Type

Store 0 into `rX` if the data in `rY` is an integer 
Store 1 into `rX` if the data in `rY` is an array 

```
rX <- type rY
```
