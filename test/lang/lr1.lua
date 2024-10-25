
local function array(r)
    local t = {}

    t.len = 0

    function t.pop()
        local r = t[t.len]
        t.len = t.len - 1
        return r
    end
    
    function t.push(v)
        t.len = t.len + 1
        t[t.len] = v
    end

    function t.shift()
        local ret = t[1]
        for i=2, t.len do
            t[i-1] = t[i]
        end
        t[t.len] = nil
        t.len = t.len - 1
        return ret
    end

    function t.sorted()
        local r = {}
        for i=1, t.len do
            local want = 1
            local value = t[i]
            while want < i and r[want] > value do
                want = want + 1
            end
            for j=want, i do
                local next_value = r[j]
                r[j] = value
                value = next_value
            end
        end
        return array(r)
    end

    function t.join(j)
        local r = ''
        for i=1, t.len do
            if i == 1 then
                r = t[i]
            else
                r = string.format('%s%s%s', r, j, t[i])
            end
        end
        return r
    end

    for i=1, #r do
        t.push(r[i])
    end

    return t
end

local function set(d)
    local s = {}
    local data = {}
    s.keys = array {}
    
    function s.add(v)
        if not s.has(v) then
            data[v] = true
            s.keys.push(v)
        end
    end

    function s.union(o)
        local r = set {}

        for i=1, s.keys.len do
            r.add(s.keys[i])
        end
        
        for i=1, o.keys.len do
            r.add(o.keys[i])
        end
        
        return r
    end

    function s.has(k)
        if data[k] then
            return true
        else
            return false
        end
    end

    function s.eq(o)
        if s.keys.len ~= o.keys.len then
            return false
        end
        for i=1, s.keys.len do
            if not o.has(s.keys[i]) then
                return false
            end
        end
        return true
    end

    for i=1, #d do
        s.add(d[i])
    end

    return s
end

local function first(g, A)
    local result = set {}
    local visited = set {}
    local stack = array {A}

    while stack.len ~= 0 do
        local x = stack.pop()
        if not visited.has(x) then
            visited.add(x)
            local first_x = string.sub(x, 1, 1)
            if not ('A' <= first_x and first_x <= 'Z') or x == '$' then
                result.add(x)
            else
                for i=1, g.len do
                    local r = g[i]
                    if r[1] == x and r[2] ~= x then
                        stack.push(r[2])
                    end
                end
            end
        end
    end

    return result
end

local function dup(items)
    local r = array {}
    for i=1, items.len do
        r.push(items[i])
    end
    return r
end

local function closure(g, items)
    local stack = dup(items)
    local state = dup(items)
    local visited = set {}

    while stack.len ~= 0 do
        local item = stack.shift()
        local key = string.format('%s,%s,%s', item.rule, item.dot, item.lookahead)
        if not visited.has(key) then
            visited.add(key)
            state.push(item)
            local r = g[item.rule]
            local x1 = r[item.dot + 1]
            local x2 = r[item.dot + 2]
            for i=1, g.len do
                if g[i][1] == x1 then
                    local ls = nil

                    if x2 then
                        ls = first(g, x1).union(first(g, x2))
                    else
                        ls = set {item.lookahead}
                    end

                    for j=1, ls.keys.len do
                        stack.push {
                            rule = i,
                            dot = 1,
                            lookahead = ls.keys[j],
                            next = -1,
                            index = -1,
                        }
                    end
                end
            end
        end
    end

    return state
end

local function key(items)
    local t = set {}
    for i=1, items.len do
        local item = items[i]
        t.add(string.format('%s,%s,%s', item.rule, item.dot, item.lookahead))
    end
    return t.keys.sorted().join('_')
end

local function states(g)
    local obj = {}

    obj.g = g
    obj.states = array {}

    local keys = {}

    function obj.add(items)
        local key = key(items)
        if not keys[key] then
            obj.states.push(items)
            keys[key] = obj.states.len
        end
        return keys[key]
    end

    function obj.has(items)
        if keys[key(items)] then
            return true
        else
            return false
        end
    end

    return obj
end

local function group_next(g, state)
    local groups = {
        map = {},
        set = set {},
    }

    for i=1, state.len do
        local s = state[i]
        local rule = g[s.rule]
        local x1 = rule[s.dot + 1]
        if x1 then
            local gs = groups.map[x1] or array {}

            gs.push {
                rule = s.rule,
                dot = s.dot + 1,
                lookahead = s.lookahead,
                index = i,
                next = -1,
            }

            groups.map[x1] = gs
            groups.set.add(x1)
        end
    end

    return groups
end

local function item_sets(g)
    local states = states(g)

    local i0v = {
        rule = 1,
        dot = 1,
        lookahead = '$',
        index = 1,
        next = -1,
    }
    local i0 = closure(g, array { i0v })
    states.add(i0)
    local stack = array { i0 }

    while stack.len ~= 0 do
        local i = stack.shift()
        local groups = group_next(g, i)
        for j=1, groups.set.keys.len do
            local t = groups.set.keys[j]
            local gs = groups.map[t]
            local j = closure(g, gs)
            if not states.has(j) then
                stack.push(j)
            end
            j = states.add(j)
            for si=1, gs.len do
                local g = gs[si]
                i[g.index].next = j
            end
        end
    end

    return states.states
end

local op_s = 1
local op_r = 2
local op_acc = 3

local function make_table(g, states)
    local action = array {}
    local go_to = array {}
    for i=1, states.len do
        action.push({})
        go_to.push({})
    end

    for i=1, states.len do
        local s = states[i]
        for j=1, s.len do
            local item = s[j]
            local gi = g[item.rule]
            if item.rule == 1
                and item.dot == gi.len
                and item.lookahead == '$' then
                action[i]['$'] = {op_acc}
            elseif item.dot == gi.len then
                local a = item.lookahead
                action[i][a] = {op_r, item.rule}
            else
                action[i][gi[item.dot + 1]] = {op_s, item.next}
            end
            local a = gi[item.dot + 1]
            if a then
                local first_a = string.sub(a, 1, 1)
                if 'A' <= first_a and first_a <= 'Z' then
                    go_to[i][a] = item.next
                end
            end
        end
    end

    return {
        action = action,
        go_to = go_to,
    }
end

local function parse(g, ag, input)
    local action = ag.action
    local go_to = ag.go_to

    local first_symbol = g[1][1]
    local offset = 1
    local stack = { 1 }
    local stack_len = 1
    local nodes = {}
    local nodes_len = 0
    local lookahead = input[1] or '$'

    while true do
        local state = stack[stack_len]
        local a = action[state][lookahead]
        if not a then
            error(string.format('err(+%s): no action for %s', offset, state))
        end

        local op = a[1]
        if op == op_s then
            nodes_len = nodes_len + 1
            nodes[nodes_len] = offset

            stack_len = stack_len + 1
            stack[stack_len] = a[2]
            offset = offset + 1 
            lookahead = input[offset] or '$'
        elseif op == op_r then
            local n = a[2]
            local symbol = g[n][1]
            local arity = g[n].len - 1
            local ast = { symbol = symbol }
            for i=1, arity do
                ast[i] = nodes[nodes_len - arity + i]
            end
            stack_len = stack_len - arity
            nodes_len = nodes_len - arity + 1
            nodes[nodes_len] = ast
            local state = stack[stack_len]
            local gt = go_to[state][symbol]
            
            if not gt then
                error('err#2')
            end

            stack_len = stack_len + 1
            stack[stack_len] = gt
        elseif op == op_acc then
            return nodes[nodes_len]
        else
            error('err#3')
        end
    end
end

local function format(ast, input)
    if type(ast) == 'table' then
        local parts = array {}
        parts.push(ast.symbol)
        for i=1, #ast do
            parts.push(format(ast[i], input))
        end
        return string.format('(%s)', parts.join(' '))
    else
        return input[ast]
    end
end

local grammar = array {
    array { 'START', 'E' },
    array { 'E', '2', 'E', 'E' },
    array { 'E', '1', 'E' },
    array { 'E', '0' },
}

local sets = item_sets(grammar)
local tab = make_table(grammar, sets)

local input = array { }

local function add(n)
    if n == 0 then
        input.push('0')
    else
        input.push('2')
        input.push('1')
        add(n-1)
        input.push('1')
        add(n-1)
    end
end

add(tonumber(arg and arg[1]) or 16)

parse(grammar, tab, input)

local input = array {'2', '1', '0', '1', '0'}

print(format(parse(grammar, tab, input), input))
