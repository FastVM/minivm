proc r0 {
    jeq [add.base] r1 0
    add r0 1
    sub r1 1
    rec r0 (r0 r1)
[add.base]
    ret r0
}

mov r1 0
mov r2 1000000
call r3 r0 (r1 r2)
println r3