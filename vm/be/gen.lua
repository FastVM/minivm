-- part: util
local typenametab = {
    i8 = 'int8_t',
    i16 = 'int16_t',
    i32 = 'int32_t',
    i64 = 'int64_t',
    u8 = 'uint8_t',
    u16 = 'uint16_t',
    u32 = 'uint32_t',
    u64 = 'uint64_t',
    f32 = 'float',
    f64 = 'double',
    func = 'vm_block_t *'
}

local function typename(name)
    return typenametab[name]
end

local function dump(name, data)
    local val = assert(io.open(name, "w"))
    val.write(val, data)
    val.close(val)
end

-- part: instrs

local prefix = 'VM_OPCODE'

local instrs = {}

local function push(op, ctype, args)
    if #args == 0 then
        args = {'void'}
    end
    local nametab = {prefix, op, ctype, table.concat(args, '_')}
    local dotab = {'do', op, ctype, table.concat(args, '_')}
    instrs[#instrs + 1] = {
        op = op,
        type = ctype,
        args = args,
        name = string.upper(table.concat(nametab, '_')),
        label = string.lower(table.concat(dotab, '_'))
    }
end

local function get_install_binary_branch(ctype)
    return function(op)
        local function install_math_const(lhs, rhs)
            push(op, ctype, {lhs, rhs, 'func', 'func'})
            push(op, ctype, {lhs, rhs, 'ptr', 'ptr'})
        end

        install_math_const('reg', 'reg')
        install_math_const('reg', 'const')
        install_math_const('const', 'reg')
        install_math_const('const', 'const')
    end
end

local function install_basic_branch(ctype)
    push('bb', ctype, {'reg', 'func', 'func'})
    push('bb', ctype, {'const', 'func', 'func'})
    push('bb', ctype, {'reg', 'ptr', 'ptr'})
    push('bb', ctype, {'const', 'ptr', 'ptr'})
    local install_branch_op = get_install_binary_branch(ctype)
    install_branch_op('beq')
    install_branch_op('blt')
end

local function get_install_binary(ctype)
    return function(op)
        local function install_math_const(lhs, rhs)
            push(op, ctype, {lhs, rhs})
        end

        install_math_const('reg', 'reg')
        install_math_const('reg', 'const')
        install_math_const('const', 'reg')
        install_math_const('const', 'const')
    end
end

local function install_basic_math(ctype)
    local install_math_op = get_install_binary(ctype)
    install_math_op('add')
    install_math_op('sub')
    install_math_op('mul')
    install_math_op('div')
    install_math_op('mod')
end

local function install_basic_extra(ctype)
    push('move', ctype, {'reg'})
    push('move', ctype, {'const'})
    push('out', ctype, {'reg'})
    push('out', ctype, {'const'})
    push('in', ctype, {})
    push('ret', ctype, {'reg'})
    push('ret', ctype, {'const'})
end

local function install_basic(ctype)
    install_basic_math(ctype)
    install_basic_branch(ctype)
    install_basic_extra(ctype)
end

local function install_basic_and_bitwise(ctype)
    install_basic(ctype)
    local install_bitwise_op = get_install_binary(ctype)
    push('bnot', ctype, {'reg'})
    push('bnot', ctype, {'const'})
    install_bitwise_op('bor')
    install_bitwise_op('bxor')
    install_bitwise_op('band')
    install_bitwise_op('bshl')
    install_bitwise_op('bshr')
end

local function install_call(nargs)
    local function install_call_with(type)
        local regs = {type}
        for i = 1, nargs do
            regs[#regs + 1] = 'reg'
        end
        push('call', 'func', regs)
    end
    install_call_with('const')
    install_call_with('reg')
end

install_basic_and_bitwise('i8')
install_basic_and_bitwise('i16')
install_basic_and_bitwise('i32')
install_basic_and_bitwise('i64')
install_basic_and_bitwise('u8')
install_basic_and_bitwise('u16')
install_basic_and_bitwise('u32')
install_basic_and_bitwise('u64')

install_basic('f32')
install_basic('f64')

push('exit', 'break', {})
push('jump', 'func', {'const'})

for i = 0, 8 do
    install_call(i)
end

-- part: emit header

do
    local lines = {}

    lines[#lines + 1] = [[
#if !defined(VM_HEADER_IR_BE_INT3)
#define VM_HEADER_IR_BE_INT3

#include <stdint.h>
#include "../ir.h"
]]

    lines[#lines + 1] = 'enum {'

    local cases = {}

    for key, instr in ipairs(instrs) do
        cases[#cases + 1] = '    ' .. instr.name .. ' = ' .. tostring(key)
    end

    lines[#lines + 1] = table.concat(cases, ',\n')

    lines[#lines + 1] = '};'

    lines[#lines + 1] = [[
struct vm_state_t;
typedef struct vm_state_t vm_state_t;

union vm_opcode_t;
typedef union vm_opcode_t vm_opcode_t;

union vm_opcode_t {
    size_t reg;
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    float f32;
    double f64;
    vm_block_t *func;
    void *ptr;
};

struct vm_state_t {
    vm_opcode_t **ips;
    size_t framesize;
    size_t nlocals;
    void *locals;
    void **ptrs;
};

vm_state_t *vm_state_init(size_t nregs);
void vm_state_deinit(vm_state_t *state);
void vm_run(vm_state_t *state, vm_block_t *block);

#endif
]]

    local incheadersrc = table.concat(lines, '\n')

    dump('vm/be/int3.h', incheadersrc)
end

-- part: emit int3.c

do
    local lines = {}

    lines[#lines + 1] = '#include "int3.h"'
    lines[#lines + 1] = '#include "value.h"'
    lines[#lines + 1] = '#include "../tag.h"'
    lines[#lines + 1] = [[

vm_state_t *vm_state_init(size_t nregs) {
    vm_state_t *ret = vm_malloc(sizeof(vm_state_t));
    ret->framesize = 256;
    ret->nlocals = nregs;
    ret->locals = vm_malloc(sizeof(vm_value_t) * (ret->nlocals));
    ret->ips = vm_malloc(sizeof(vm_opcode_t *) * (ret->nlocals / ret->framesize));
    return ret;
}

void vm_state_deinit(vm_state_t *state) {
    vm_free(state->ips);
    vm_free(state->locals);
    vm_free(state);
}
]]

    local simplebinary = {
        add = '+',
        sub = '-',
        mul = '*',
        div = '/',
        bor = '|',
        bxor = '^',
        band = '&',
        bshl = '>>',
        bshr = '<<'
    }

    local simplebranch = {
        beq = '==',
        blt = '<',
    }

    local isinttype = {
        i8 = true,
        i16 = true,
        i32 = true,
        i64 = true,
        u8 = true,
        u16 = true,
        u32 = true,
        u64 = true,
        f32 = false,
        f64 = false
    }

    do
        local binaryint = {'add', 'sub', 'mul', 'div', 'mod', 'bor', 'bxor', 'band', 'bshl', 'bshr'}
        local binaryfloat = {'add', 'sub', 'mul', 'div', 'mod'}
        local binarytypes = {'i8', 'i16', 'i32', 'i64', 'u8', 'u16', 'u32', 'u64', 'f32', 'f64'}


        local map = {
            reg = '== VM_ARG_REG',
            const = '!= VM_ARG_REG'
        }

        local kinds = {{'reg', 'reg'}, {'reg', 'const'}, {'const', 'reg'}, {'const', 'const'}}

        lines[#lines + 1] = 'vm_opcode_t *vm_run_comp(vm_state_t *state, vm_block_t *block) {'
        lines[#lines + 1] = '    if (block->cache) {'
        lines[#lines + 1] = '        return block->cache;'
        lines[#lines + 1] = '    }'
        lines[#lines + 1] = '    size_t aops = 64;'
        lines[#lines + 1] = '    vm_opcode_t *ops = vm_malloc(sizeof(vm_opcode_t) * aops);'
        lines[#lines + 1] = '    size_t nops = 0;'
        lines[#lines + 1] = '    for (size_t ninstr = 0; ninstr < block->len; ninstr++) {'
        lines[#lines + 1] = '        if (nops + 32 >= aops) {'
        lines[#lines + 1] = '            aops = (nops + 32) * 2;'
        lines[#lines + 1] = '            ops = vm_realloc(ops, sizeof(vm_opcode_t) * aops);'
        lines[#lines + 1] = '        }'
        lines[#lines + 1] = '        vm_instr_t instr = block->instrs[ninstr];'
        lines[#lines + 1] = '        switch (instr.op) {'
        do
            lines[#lines + 1] = '        case VM_IOP_MOVE: {'
            lines[#lines + 1] = '            if (instr.out.type == VM_ARG_NONE) {'
            lines[#lines + 1] = '                break;'
            lines[#lines + 1] = '            }'
            for tkey, tvalue in ipairs(binarytypes) do
                lines[#lines + 1] = '            if (instr.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                for _, a0type in ipairs({'reg', 'const'}) do
                    lines[#lines + 1] = '                if (instr.args[0].type ' .. map[a0type] .. ') {'
                    local name = string.upper(table.concat({prefix, 'move', tvalue, a0type}, '_'))
                    lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                    if a0type == 'reg' then
                        lines[#lines + 1] = '                    ops[nops++].reg = instr.args[0].reg;'
                    else
                        lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                typename(tvalue) .. ') instr.args[0].num;'
                    end
                    lines[#lines + 1] = '                    ops[nops++].reg = instr.out.reg;'
                    lines[#lines + 1] = '                    break;'
                    lines[#lines + 1] = '                }'
                end
                lines[#lines + 1] = '            }'
            end
            lines[#lines + 1] = '            goto err;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_IOP_BNOT: {'
            lines[#lines + 1] = '            if (instr.out.type == VM_ARG_NONE) {'
            lines[#lines + 1] = '                break;'
            lines[#lines + 1] = '            }'
            for tkey, tvalue in ipairs(binarytypes) do
                if isinttype[tvalue] then
                    lines[#lines + 1] = '            if (instr.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                    for _, a0type in ipairs({'reg', 'const'}) do
                        lines[#lines + 1] = '                if (instr.args[0].type ' .. map[a0type] .. ') {'
                        local name = string.upper(table.concat({prefix, 'bnot', tvalue, a0type}, '_'))
                        lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                        if a0type == 'reg' then
                            lines[#lines + 1] = '                    ops[nops++].reg = instr.args[0].reg;'
                        else
                            lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                    typename(tvalue) .. ') instr.args[0].num;'
                        end
                        lines[#lines + 1] = '                    ops[nops++].reg = instr.out.reg;'
                        lines[#lines + 1] = '                    break;'
                        lines[#lines + 1] = '                }'
                    end
                    lines[#lines + 1] = '            }'
                end
            end
            lines[#lines + 1] = '            goto err;'
            lines[#lines + 1] = '        }'
        end
        for key, value in ipairs(binaryint) do
            lines[#lines + 1] = '        case VM_IOP_' .. string.upper(value) .. ': {'
            lines[#lines + 1] = '            if (instr.out.type == VM_ARG_NONE) {'
            lines[#lines + 1] = '                break;'
            lines[#lines + 1] = '            }'
            for tkey, tvalue in ipairs(binarytypes) do
                if isinttype[tvalue] or value == 'add' or value == 'sub' or value == 'mul' or value == 'div' or value == 'mod' then
                    lines[#lines + 1] = '            if (instr.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                    for _, pair in ipairs(kinds) do
                        lines[#lines + 1] = '                if (instr.args[0].type ' .. map[pair[1]] .. ' && ' ..
                                                'instr.args[1].type ' .. map[pair[2]] .. ') {'
                        local name = string.upper(table.concat({prefix, value, tvalue, pair[1], pair[2]}, '_'))
                        lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                        if pair[1] == 'reg' then
                            lines[#lines + 1] = '                    ops[nops++].reg = instr.args[0].reg;'
                        else
                            lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                    typename(tvalue) .. ') instr.args[0].num;'
                        end
                        if pair[2] == 'reg' then
                            lines[#lines + 1] = '                    ops[nops++].reg = instr.args[1].reg;'
                        else
                            lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                    typename(tvalue) .. ') instr.args[1].num;'
                        end
                        lines[#lines + 1] = '                    ops[nops++].reg = instr.out.reg;'
                        lines[#lines + 1] = '                    break;'
                        lines[#lines + 1] = '                }'
                    end
                    lines[#lines + 1] = '            }'
                end
            end
            lines[#lines + 1] = '            goto err;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_IOP_IN: {'
            lines[#lines + 1] = '            if (instr.out.type == VM_ARG_NONE) {'
            lines[#lines + 1] = '                break;'
            lines[#lines + 1] = '            }'
            for tkey, tvalue in ipairs(binarytypes) do
                lines[#lines + 1] = '            if (instr.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                local name = string.upper(table.concat({prefix, 'in', tvalue, 'void'}, '_'))
                lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                lines[#lines + 1] = '                    ops[nops++].reg = instr.args[0].reg;'
                lines[#lines + 1] = '            }'
            end
            lines[#lines + 1] = '            break;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_IOP_CALL: {'
            for nargs=0, 8 do
                for _, val in ipairs({'const', 'reg'}) do
                    local name = {prefix, 'call', 'func', val}
                    while #name - 4 < nargs do
                        name[#name+1] = 'reg'
                    end
                    name = string.upper(table.concat(name, '_'))
                    lines[#lines + 1] = '            if (instr.args[' .. tostring(nargs+1) .. '].type == VM_ARG_NONE) {'
                    lines[#lines + 1] = '                ops[nops++].ptr = state->ptrs[' .. name .. '];'
                    if val == 'reg' then
                        lines[#lines + 1] = '                ops[nops++].reg = instr.args[0].reg;'
                    else
                        lines[#lines + 1] = '                ops[nops++].func = instr.args[0].func;'
                    end
                    for argno=1, nargs do
                        lines[#lines + 1] = '                ops[nops++].reg = instr.args[' .. tostring(argno) .. '].reg;'
                    end
                    lines[#lines + 1] = '                if (instr.out.type == VM_ARG_NONE) {'
                    lines[#lines + 1] = '                        ops[nops++].reg = 256;'
                    lines[#lines + 1] = '                } else {'
                    lines[#lines + 1] = '                        ops[nops++].reg = instr.out.reg;'
                    lines[#lines + 1] = '                }'
                    lines[#lines + 1] = '                break;'
                    lines[#lines + 1] = '            }'
                end
            end
            lines[#lines + 1] = '            goto err;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_IOP_OUT: {'
            for tkey, tvalue in ipairs(binarytypes) do
                lines[#lines + 1] = '            if (instr.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                for _, val in ipairs({'const', 'reg'}) do
                    local name = string.upper(table.concat({prefix, 'out', tvalue, val}, '_'))
                    lines[#lines + 1] = '                if (instr.args[0].type ' .. map[val] .. ') {'
                    lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                    if val == 'reg' then
                        lines[#lines + 1] = '                    ops[nops++].reg = instr.args[0].reg;'
                    else
                        lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..typename(tvalue) .. ') instr.args[0].num;'
                    end
                    lines[#lines + 1] = '                    break;'
                    lines[#lines + 1] = '                }'
                end
                lines[#lines + 1] = '            }'
            end
            lines[#lines + 1] = '        }'
            lines[#lines + 1] = '        default: goto err;'
        end
        lines[#lines + 1] = '         }'
        lines[#lines + 1] = '     }'
        lines[#lines + 1] = '     switch (block->branch.op) {'
        do
            lines[#lines + 1] = '        case VM_BOP_EXIT: {'
            local name = string.upper(table.concat({prefix, 'exit', 'break', 'void'}, '_'))
            lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
            lines[#lines + 1] = '            break;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_BOP_JUMP: {'
            local name = string.upper(table.concat({prefix, 'jump', 'func', 'const'}, '_'))
            lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
            lines[#lines + 1] = '                    ops[nops++].func = block->branch.targets[0];'
            lines[#lines + 1] = '            break;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_BOP_RET: {'
            for tkey, tvalue in ipairs(binarytypes) do
                lines[#lines + 1] = '            if (block->branch.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                for _, val in ipairs({'const', 'reg'}) do
                    lines[#lines + 1] = '                if (block->branch.args[0].type ' .. map[val] .. ') {'
                    local name = string.upper(table.concat({prefix, 'ret', tvalue, val}, '_'))
                    lines[#lines + 1] = '                   ops[nops++].ptr = state->ptrs[' .. name .. '];'
                    if val == 'reg' then
                        lines[#lines + 1] = '                   ops[nops++].reg = block->branch.args[0].reg;'
                    else
                        lines[#lines + 1] = '                   ops[nops++].' .. tvalue .. ' = (' .. typename(tvalue) .. ') block->branch.args[0].num;'
                    end
                    lines[#lines + 1] = '                   break;'
                    lines[#lines + 1] = '               }'
                end
                lines[#lines + 1] = '            }'
            end
            lines[#lines + 1] = '            goto err;'
            lines[#lines + 1] = '        }'
        end
        do
            lines[#lines + 1] = '        case VM_BOP_BB: {'
            for tkey, tvalue in ipairs(binarytypes) do
                lines[#lines + 1] = '                if (block->branch.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                for _, val in ipairs({'const', 'reg'}) do
                    lines[#lines + 1] = '                    if (block->branch.args[0].type ' .. map[val] .. ') {'
                    local name = string.upper(table.concat({prefix, 'bb', tvalue, val, 'func', 'func'}, '_'))
                    lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                    if val == 'reg' then
                        lines[#lines + 1] = '                    ops[nops++].reg = block->branch.args[0].reg;'
                    else
                        lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                typename(tvalue) .. ') block->branch.args[0].num;'
                    end
                    lines[#lines + 1] = '                    ops[nops++].func = block->branch.targets[0];'
                    lines[#lines + 1] = '                    ops[nops++].func = block->branch.targets[1];'
                    lines[#lines + 1] = '                    break;'
                    lines[#lines + 1] = '                    }'
                end
                lines[#lines + 1] = '                    break;'
                lines[#lines + 1] = '                }'
            end
            lines[#lines + 1] = '             goto err;'
            lines[#lines + 1] = '        }'
        end
        do
            for key, value in ipairs({'blt', 'beq'}) do
                lines[#lines + 1] = '        case VM_BOP_' .. string.upper(value) .. ': {'
                for tkey, tvalue in ipairs(binarytypes) do
                    lines[#lines + 1] = '            if (block->branch.tag == VM_TAG_' .. string.upper(tvalue) .. ') {'
                    for _, pair in ipairs(kinds) do
                        lines[#lines + 1] = '                if (block->branch.args[0].type ' .. map[pair[1]] .. ' && ' ..
                                                'block->branch.args[1].type ' .. map[pair[2]] .. ') {'
                        local name = string.upper(table.concat({prefix, value, tvalue, pair[1], pair[2], 'func', 'func'}, '_'))
                        lines[#lines + 1] = '                    ops[nops++].ptr = state->ptrs[' .. name .. '];'
                        if pair[1] == 'reg' then
                            lines[#lines + 1] = '                    ops[nops++].reg = block->branch.args[0].reg;'
                        else
                            lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                    typename(tvalue) .. ') block->branch.args[0].num;'
                        end
                        if pair[2] == 'reg' then
                            lines[#lines + 1] = '                    ops[nops++].reg = block->branch.args[1].reg;'
                        else
                            lines[#lines + 1] = '                    ops[nops++].' .. tvalue .. ' = (' ..
                                                    typename(tvalue) .. ') block->branch.args[1].num;'
                        end
                        lines[#lines + 1] = '                    ops[nops++].func = block->branch.targets[0];'
                        lines[#lines + 1] = '                    ops[nops++].func = block->branch.targets[1];'
                        lines[#lines + 1] = '                    break;'
                        lines[#lines + 1] = '                }'
                    end
                    lines[#lines + 1] = '            }'
                end
                lines[#lines + 1] = '             goto err;'
                lines[#lines + 1] = '        }'
            end
        end
        do
            lines[#lines + 1] = '        default: goto err;'
        end
        lines[#lines + 1] = '     }'
        lines[#lines + 1] = '     block->cache = ops;'
        lines[#lines + 1] = '     return ops;'
        lines[#lines + 1] = 'err:;'
        lines[#lines + 1] = '     fprintf(stderr, "BAD INSTR!\\n");'
        lines[#lines + 1] = '     __builtin_trap();'
        lines[#lines + 1] = '}'
    end
    lines[#lines + 1] = 'void vm_run(vm_state_t *state, vm_block_t *block) {'

    do
        local cases = {}

        lines[#lines + 1] = '    void *ptrs[] = {'
        for key, instr in ipairs(instrs) do
            cases[#cases + 1] = '        [' .. instr.name .. '] = &&' .. instr.label
        end

        lines[#lines + 1] = table.concat(cases, ',\n')
        lines[#lines + 1] = '    };'
    end

    lines[#lines + 1] = '    state->ptrs = ptrs;'
    lines[#lines + 1] = '    vm_opcode_t *ip = vm_run_comp(state, block);'
    lines[#lines + 1] = '    vm_value_t *locals = state->locals;'
    lines[#lines + 1] = '    vm_opcode_t **ips = state->ips;'
    lines[#lines + 1] = '    goto *(ip++)->ptr;'

    do
        local cases = {}

        for key, instr in ipairs(instrs) do
            local case = {}

            case[#case + 1] = '    ' .. instr.label .. ': {'
            
            -- case[#case+1] = '        printf("' .. instr.name .. '\\n");'
            
            if simplebinary[instr.op] then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                if instr.args[2] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a1 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a1 = (ip++)->' .. instr.type .. ';'
                end
                -- case[#case+1] = '        printf("%zi ' ..simplebinary[instr.op] .. ' %zi\\n", (ptrdiff_t) a0, (ptrdiff_t) a1);'
                case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = a0 ' .. simplebinary[instr.op] ..
                                      ' a1;'
            elseif instr.op == 'mod' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                if instr.args[2] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a1 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a1 = (ip++)->' .. instr.type .. ';'
                end
                if instr.type == 'f64' then
                    case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = fmod(a0, a1);'
                elseif instr.type == 'f32' then
                    case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = fmodf(a0, a1);'
                else
                    case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = a0 % a1;'
                end
            elseif instr.op == 'move' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = a0;'
            elseif instr.op == 'bnot' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = ~a0;'
            elseif instr.op == 'in' then
                local tname = typename(instr.type)
                case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = (' .. tname.. ') fgetc(stdin);'
            elseif instr.op == 'out' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                case[#case + 1] = '        putchar((int) a0);'
            elseif instr.op == 'jump' then
                case[#case + 1] = '        vm_block_t *t0 = (ip++)->func;'
                case[#case + 1] = '        ip = vm_run_comp(state, t0);'
            elseif instr.op == 'bb' and instr.args[2] == 'ptr' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[ip[0].reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = ip[0].' .. instr.type .. ';'
                end
                case[#case + 1] = '        if (a0) {'
                case[#case + 1] = '            ip = ip[1].ptr;'
                case[#case + 1] = '        } else {'
                case[#case + 1] = '            ip = ip[2].ptr;'
                case[#case + 1] = '        }'
            elseif instr.op == 'bb' and instr.args[2] == 'func' then
                case[#case + 1] = '        vm_opcode_t *head = ip-1;'
                case[#case + 1] = '        head->ptr = &&' .. table.concat({'do', instr.op, instr.type, instr.args[1], 'ptr', 'ptr'}, '_') .. ';'
                case[#case + 1] = '        ip[1].ptr = vm_run_comp(state, ip[1].func);'
                case[#case + 1] = '        ip = head;'
            elseif (instr.op == 'blt' or instr.op == 'beq') and instr.args[3] == 'ptr' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[ip[0].reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = ip[0].' .. instr.type .. ';'
                end
                if instr.args[0] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a1 = locals[ip[1].reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a1 = ip[1].' .. instr.type .. ';'
                end
                case[#case + 1] = '        if (a0 ' .. simplebranch[instr.op] .. ' a1) {'
                case[#case + 1] = '            ip = ip[2].ptr;'
                case[#case + 1] = '        } else {'
                case[#case + 1] = '            ip = ip[3].ptr;'
                case[#case + 1] = '        }'
            elseif (instr.op == 'blt' or instr.op == 'beq') and instr.args[3] == 'func' then
                case[#case + 1] = '        vm_opcode_t *head = ip-1;'
                case[#case + 1] = '        head->ptr = &&' .. table.concat({'do', instr.op, instr.type, instr.args[1], instr.args[2], 'ptr', 'ptr'}, '_') .. ';'
                case[#case + 1] = '        ip[2].ptr = vm_run_comp(state, ip[2].func);'
                case[#case + 1] = '        ip[3].ptr = vm_run_comp(state, ip[3].func);'
                case[#case + 1] = '        ip = head;'
            elseif instr.op == 'ret' then
                local tname = typename(instr.type)
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        ' .. tname .. ' a0 = locals[(ip++)->reg].' .. instr.type .. ';'
                else
                    case[#case + 1] = '        ' .. tname .. ' a0 = (ip++)->' .. instr.type .. ';'
                end
                case[#case + 1] = '        locals -= 256;'
                case[#case + 1] = '        ip = *(--ips);'
                case[#case + 1] = '        locals[(ip++)->reg].' .. instr.type .. ' = (' .. tname.. ') a0;'
            elseif instr.op == 'call' then
                if instr.args[1] == 'reg' then
                    case[#case + 1] = '        vm_block_t *t0 = locals[(ip++)->reg].func;'
                else
                    case[#case + 1] = '        vm_block_t *t0 = (ip++)->func;'
                end
                for argno=1, #instr.args-1 do
                    case[#case + 1] = '        locals[' .. tostring(argno+256) .. '] = locals[(ip++)->reg];'
                end
                case[#case + 1] = '        locals += 256;'
                case[#case + 1] = '        *(ips++) = ip;'
                case[#case + 1] = '        ip = vm_run_comp(state, t0);'
            elseif instr.op == 'exit' then
                case[#case + 1] = '        return;'
            else
                case[#case + 1] = '        fprintf(stderr, "unimplemend label: ' .. instr.name .. '\\n");'
                case[#case + 1] = '        __builtin_trap();'
            end

            case[#case + 1] = '        goto *(ip++)->ptr;'
            case[#case + 1] = '    }'

            cases[#cases + 1] = table.concat(case, '\n')
        end

        lines[#lines + 1] = table.concat(cases, '\n')
    end

    lines[#lines + 1] = '}'

    local incheadersrc = table.concat(lines, '\n')

    dump('vm/be/int3.c', incheadersrc)
end
