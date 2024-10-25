function first(G, A) {
    let result = new Set
    let visited = new Set
    let stack = [A]
    while (stack.length > 0) {
        let X = stack.pop()
        if (visited.has(X)) continue
        visited.add(X)
        if (!/^[A-Z]/.test(X) || X === '$') {
            result.add(X)
            continue
        }
        for (let i = 0; i < G.length; i++) {
            let r = G[i]
            if (r[0] === X && r[1] !== X) {
                stack.push(r[1])
            }
        }
    }
    return result
}

function closure(G, items) {
    let stack = items.slice()
    let state = items.slice()
    let visited = new Set
    while (true) {
        if (stack.length === 0) break
        let item = stack.shift()
        let key = item.rule + ',' + item.dot + ',' + item.lookahead
        if (visited.has(key)) {
            continue
        }
        visited.add(key)
        state.push(item)
        let r = G[item.rule]
        let X1 = r[item.dot + 1]
        let X2 = r[item.dot + 2]
        for (let i = 0; i < G.length; i++) {
            if (G[i][0] !== X1) continue
            let ls = item.dot + 2 === r.length
                ? new Set([item.lookahead])
                : union(first(G, X1), first(G, X2))
            for (let lookahead of ls) {
                let nitem = {
                    rule: i,
                    dot: 0,
                    lookahead,
                    next: -1,
                    index: -1,
                }
                stack.push(nitem)
            }
        }
    }
    return state
}

function union(A, B) {
    // :(
    return new Set(Array.from(A).concat(Array.from(B)))
}

class States {
    static key(items) {
        return items.map(item => {
            return item.rule + ',' + item.dot + ',' + item.lookahead
        }).sort().join('_')
    }
    constructor(G) {
        this.G = G
        this.states = []
        this._stateKeys = new Map
    }
    add(items) {
        let key = States.key(items)

        let i = this._stateKeys.get(key)
        if (i !== undefined) return i
        i = this.states.length
        this.states.push(items)
        this._stateKeys.set(key, i)
        return i
    }
    has(items) {
        return this._stateKeys.has(States.key(items))
    }
}

function groupNext(G, state) {
    let groups = new Map
    for (let i = 0; i < state.length; i++) {
        let s = state[i]
        let rule = G[s.rule]
        if (s.dot + 1 >= rule.length) continue
        let X0 = rule[0]
        let X1 = rule[s.dot + 1]
        let gs = groups.get(X1)
        if (gs == null) gs = []
        gs.push({
            rule: s.rule,
            dot: s.dot + 1,
            lookahead: s.lookahead,
            index: i,
            next: -1,
        })
        groups.set(X1, gs)
    }
    return groups
}

function itemSets(G) {
    let states = new States(G)
    let I0 = closure(G, [{ rule: 0, dot: 0, lookahead: '$', index: 0, next: -1 }])
    let i0 = states.add(I0)
    let stack = [I0]
    while (true) {
        if (stack.length === 0) break
        let I = stack.shift()
        let groups = groupNext(G, I)
        for (let T of Array.from(groups.keys()).sort()) {
            let gs = groups.get(T)
            let J = closure(G, gs)
            if (!states.has(J)) stack.push(J)
            let j = states.add(J)
            for (let si = 0; si < gs.length; si++) {
                let g = gs[si]
                I[g.index].next = j
            }
        }
    }
    return states.states
}

function createTable(G, states) {
    let action = Array(states.length).fill(null).map(x => new Map)
    let goTo = Array(states.length).fill(null).map(x => new Map)

    for (let i = 0; i < states.length; i++) {
        let I = states[i]
        for (let j = 0; j < I.length; j++) {
            let item = I[j]
            if (item.rule === 0 && item.dot + 1 === G[item.rule].length && item.lookahead === '$') {
                action[i].set('$', 'acc')
            } else if (item.dot + 1 === G[item.rule].length) {
                let a = item.lookahead
                action[i].set(a, 'r' + item.rule)
            } else {
                let a = G[item.rule][item.dot + 1]
                action[i].set(a, 's' + item.next)
            }
            let A = G[item.rule][item.dot + 1]
            if (/^[A-Z]/.test(A)) {
                goTo[i].set(A, item.next)
            }
        }
    }
    return { action, goTo }
}

function parse(G, { action, goTo }, input) {
    let firstSymbol = G[0][0]
    let offset = 0
    let stack = [0]
    let nodes = []
    let lookahead = input[0]

    while (true) {
        let state = stack[stack.length - 1]
        let a = action[state].get(lookahead)
        if (a === undefined) throw new Error(`no action for state: ${state} lookahead: ${lookahead}`)
        let n = Number(a.slice(1))
        if (/^s/.test(a)) {
            nodes.push({ symbol: lookahead, offset })
            stack.push(n)
            lookahead = input[++offset]
            if (lookahead == null) {
                lookahead = '$'
            }
        } else if (/^r/.test(a)) {
            let symbol = G[n][0]
            let arity = G[n].length - 1
            let children = Array(arity)
            for (let i = 0; i < arity; i++) {
                children[arity - 1 - i] = nodes.pop()
                stack.pop()
            }
            nodes.push({ symbol, children })

            state = stack[stack.length - 1]
            let gt = goTo[state].get(symbol)
            if (gt === undefined) throw new Error(`undefined goto for state=${state} symbol=${symbol}`)
            stack.push(gt)
        } else if (a === 'acc') {
            return nodes.pop()
        } else {
            throw new Error(`unexpected value for action ${a}`)
        }
    }
}

function format(ast) {
    if (ast.children != null) {
        return '(' + [ast.symbol, ...ast.children.map(format)].join(' ') + ')'
    } else {
        return ast.symbol
    }
} 

let grammar = [
    [ "START", "E" ],
    [ "E", "2", "E", "E" ],
    [ "E", "1", "E" ],
    [ "E", "0" ],
]
let sets = itemSets(grammar)
// console.log(sets)
let tab = createTable(grammar, sets)

let input = []

function add(n) {
    if (n == 0) {
        input.push('0')
    } else {
        input.push('2')
        input.push('1')
        add(n-1)
        input.push('1')
        add(n-1)
    }
}

add(process.argv[2] || 16)

parse(grammar, tab, input)

console.log(format(parse(grammar, tab, ['2', '1', '0', '1', '0'])))

// console.log(tab)
