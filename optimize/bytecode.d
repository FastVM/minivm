module optimize.bytecode;

import core.memory;

alias Number = double;
alias String = immutable(char)*;

extern (C) void vm_run(void* func);

enum Opcode : ubyte {
    exit,
    store_reg,
    store_byte,
    store_int,
    store_fun,
    fun_done,
    equal,
    equal_num,
    not_equal,
    not_equal_num,
    less,
    less_num,
    greater,
    greater_num,
    less_than_equal,
    less_than_equal_num,
    greater_than_equal,
    greater_than_equal_num,
    jump_always,
    jump_if_false,
    jump_if_true,
    jump_if_equal,
    jump_if_equal_num,
    jump_if_not_equal,
    jump_if_not_equal_num,
    jump_if_less,
    jump_if_less_num,
    jump_if_greater,
    jump_if_greater_num,
    jump_if_less_than_equal,
    jump_if_less_than_equal_num,
    jump_if_greater_than_equal,
    jump_if_greater_than_equal_num,
    inc,
    inc_num,
    dec,
    dec_num,
    add,
    add_num,
    sub,
    sub_num,
    mul,
    mul_num,
    div,
    div_num,
    mod,
    mod_num,
    static_call0,
    static_call1,
    static_call2,
    static_call,
    rec0,
    rec1,
    rec2,
    rec,
    call0,
    call1,
    call2,
    call,
    ret,
    println,
    putchar,
    array,
    length,
    index,
    syscall,
}

bool noOutputs(Opcode op) {
    return op == Opcode.putchar 
        || op == Opcode.println 
        || op == Opcode.ret
        || op == Opcode.exit;
}

bool isJump(Opcode op) {
    return op == Opcode.jump_always
        || op == Opcode.jump_if_false 
        || op == Opcode.jump_if_true 
        || op == Opcode.jump_if_equal 
        || op == Opcode.jump_if_equal_num 
        || op == Opcode.jump_if_not_equal 
        || op == Opcode.jump_if_not_equal_num 
        || op == Opcode.jump_if_less 
        || op == Opcode.jump_if_less_num 
        || op == Opcode.jump_if_greater 
        || op == Opcode.jump_if_greater_num 
        || op == Opcode.jump_if_less_than_equal 
        || op == Opcode.jump_if_less_than_equal_num 
        || op == Opcode.jump_if_greater_than_equal 
        || op == Opcode.jump_if_greater_than_equal_num;
}