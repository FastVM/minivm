proc r0 {
[add.redo]
    jeq [add.base] r1 0
    add r0 1
    sub r1 1
    j [add.redo]
[add.base]
    ret r0
}

mov r1 0
mov r2 1000000000
call r3 r0 (r1 r2)
println r3