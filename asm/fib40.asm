proc r0 {
    jlt [fib.lt] r0 2
    sub r0 1
    rec r1 (r0)
    sub r0 1
    rec r0 (r0)
    add r0 r1
[fib.lt]
    ret r0
}
mov r1 40
call r1 r0(r1)
println r1