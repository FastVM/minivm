
    proc r0 {
        jle [tree.base] r1 0
    [tree.rec]
        sub r1 1
        add r2 r0 r0
        sub r3 r2 1
        rec r4 (r3 r1)
        rec r5 (r2 r1)
        new r6 (r0 r4 r5)
        ret r6
    [tree.base]
        new r2 (r0)
        ret r2
    }

    proc r1 {
        len r1 r0
        jne [check.base] r1 3
    [check.rec]
        nth r2 r0 0
        nth r3 r0 1
        nth r4 r0 2
        rec r3 (r3)
        rec r4 (r4)
        add r2 r3
        sub r2 r4
        ret r2
    [check.base]
        nth r2 r0 0
        ret r2
    }

    proc r11 {
        jlt [pow2n.done] r1 0
    [pow2n.rec]
        add r0 r0
        sub r1 1
        rec r2 (r0 r1)
        ret r2
    [pow2n.done]
        ret r0
    }

    mov r2 12
    mov r3 4
    add r4 r3 2
    
    jgt [set.skip] r4 r2
    mov r4 r2
[set.skip]

    mov r5 0
    add r6 r4 1
    call r7 r0(r5 r6)  
    call r8 r1(r7)
    mov r9 1
    new r10 (r6 r8)  
    println r10 

    mov r18 0
    call r19 r0(r18 r4)

    mov r5 r3
[loop.redo]
    jgt [loop.done] r5 r4
    mov r6 1
    sub r7 r4 r5
    add r7 r3
    call r8 r11(r6 r7)
    mov r9 0
    mov r10 1
[inner.redo]
    jgt [inner.done] r10 r8
    mov r12 1
    call r13 r0(r12 r5)
    call r14 r1(r13)
    mov r15 -1
    call r16 r0(r15 r5)
    call r17 r1(r16)
    add r9 r14
    add r9 r17 
    add r10 1
    j [inner.redo]
[inner.done]
    new r12 (r8 r5 r9)
    println r12
    add r5 2
    j [loop.redo]
[loop.done]
    
    call r20 r1(r19)
    new r21 (r4 r20)  
    println r21
