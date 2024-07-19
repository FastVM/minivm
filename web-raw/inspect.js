
const VM_TAG_UNK = 0;
const VM_TAG_NIL = 1;
const VM_TAG_BOOL = 2;
const VM_TAG_I8 = 3;
const VM_TAG_I16 = 4;
const VM_TAG_I32 = 5;
const VM_TAG_I64 = 6;
const VM_TAG_F32 = 7;
const VM_TAG_NUMBER = 8;
const VM_TAG_STR = 9;
const VM_TAG_CLOSURE = 10;
const VM_TAG_FUN = 11;
const VM_TAG_TAB = 12;
const VM_TAG_FFI = 13;

export class FFI {
    constructor(n) {
        this.n = n;
    }
}

export class Fun {
    constructor(n) {
        this.n = n;
    }
}

export class Closure extends Array {}

export class InspectError extends Error {}

export const inspect = (bytes) => {
    const values = [];
    let head = 0;

    const sleb = () => {
        let shift = 0n;
        let result = 0n;
        while (true) {
            let byte = BigInt(bytes[head++]);
            result |= (byte & 0x7Fn) << shift;
            shift += 7n;
            if ((0x80n & byte) === 0n) {
                if (shift < 64n && (byte & 0x40n) !== 0n) {
                    return result | -(1n << shift);
                }
                return result;
            }
        }
    };
    
    const ulebn = () => {
        let x = 0n;
        let shift = 0n;
        while (true) {
            let buf = BigInt(bytes[head++]);
            x |= (buf & 0x7Fn) << shift;
            if (buf < 0x80n) {
                return x;
            }
            shift += 7n;
        }
    };

    const uleb = () => {
        return Number(ulebn());
    };

    let n = 0;
    stop: while (true) {
        const tag = bytes[head++];
        let value;
        switch (tag) {
            default: {
                throw new InspectError(); 
            }
            case VM_TAG_UNK: {
                break stop;
            }
            case VM_TAG_NIL: {
                value = null;
                break;
            }
            case VM_TAG_BOOL: {
                value = bytes[head++] != 0;
                break;
            }
            case VM_TAG_I8:
            case VM_TAG_I16:
            case VM_TAG_I32:
            case VM_TAG_I64: {
                value = sleb();
                break;
            }
            case VM_TAG_FUN: {
                value = new Fun(sleb());
                break;
            }
            case VM_TAG_F32: {
                const u32s = new Uint32Array(1);
                u32s[0] = ulebn();
                const f32s = new Float32Array(u32s.buffer);
                value = f32s[0];
                break;
            }
            case VM_TAG_NUMBER: {
                const u64s = new BigUint64Array(1);
                u64s[0] = ulebn();
                const f64s = new Float64Array(u64s.buffer);
                value = f64s[0];
                break;
            }
            case VM_TAG_STR: {
                let len = uleb();
                let buf = ''
                for (let i = 0; i < len; i++) {
                    buf += String.fromCharCode(bytes[head++]);
                }
                value = buf;
                break;
            }
            case VM_TAG_FFI: {
                value = new FFI(uleb());
                break;
            }
            case VM_TAG_CLOSURE: {
                let len = uleb();
                let table = new Closure();
                for (let i = 0; i < len; i++) {
                    table.push(uleb());
                }
                value = table;
                break;
            }
            case VM_TAG_TAB: {
                let len = uleb();
                let table = new Map();
                for (let i = 0; i < len; i++) {
                    table.set(uleb(), uleb());
                }
                value = table;
                break;
            }
        }
        values.push(value);
        n += 1;
    }

    for (const value of Object.values(values)) {
        if (value instanceof Closure) {
            for (let i = 0; i < value.length; i++) {
                value[i] = values[value[i]];
            }
        } else if (value instanceof Map) {
            const ents = [...value.entries()];
            for (const [k] of ents) {
                value.delete(k);
            }
            for (const [k, v] of ents) {
                if (values[k] != null && values[v] != null) {
                    value.set(values[k], values[v]);
                }
            }
        }
    };
    
    return values;
};

export const render = (tree, open=new Set()) => {
    if (tree instanceof Uint8Array) {
        tree = inspect(tree)[0];
    }

    const body = document.createElement('div');

    const depth = (elem) => {
        let depth = 0;
        for (let cur = elem; cur != body && cur != null; cur = cur.parentElement) {
            depth += 1;
        }
        return depth;
    };

    const more = (path, name, tree, elem) => {
        const pad = depth(elem) ? 2 : 0;

        if (tree instanceof Map) {
            const details = document.createElement('details');
            const inner = document.createElement('div');
            const summary = document.createElement('summary');

            details.appendChild(summary);
            details.appendChild(inner);
            elem.appendChild(details);

            summary.innerText = name;
            inner.style.display = 'flex';
            inner.style.flexDirection = 'column';

            const openDetails = () => {
                details.open = true;

                const nums = [];
                const strs = [];
                for (const k of tree.keys()) {
                    if (typeof k === 'number') {
                        nums.push(Number(k));
                    }
                    if (typeof k === 'string') {
                        strs.push(k);
                    }
                }
                nums.sort((x, y) => x - y);
                for (const k of nums) {
                    more(`${path}[${k}]`, String(k), tree.get(k), inner);
                }
                strs.sort();
                for (const k of strs) {
                    more(`${path}.${k}`, k, tree.get(k), inner);
                }
                let n = 0;
                for (const [k, v] of tree.entries()) {
                    if (typeof k !== 'number' && typeof k !== 'string') {
                        more(`${path}::k${n}`, 'key', k, inner);
                        more(`${path}::v${n}`, 'value', v, inner);
                        n += 1;
                    }
                }
            };

            details.onclick = (e) => {
                if (e.target !== summary) {
                    return;
                }

                if (details.open) {
                    inner.innerHTML = '';
                    open.delete(path);
                    return;
                }

                open.add(path);

                openDetails();
            };
            
            if (open.has(path)) {
                openDetails();
            }

            details.style.paddingLeft = `${pad}em`;
        } else if (tree instanceof Closure) {
            const details = document.createElement('details');
            const inner = document.createElement('div');
            const summary = document.createElement('summary');
            
            details.appendChild(summary);
            details.appendChild(inner);
            elem.appendChild(details);

            summary.innerText = name;
            inner.style.display = 'flex';
            inner.style.flexDirection = 'column';

            const openDetails = () => {
                details.open = true;

                open.add(path);

                let n = 0;
                for (const v of tree) {
                    more(`${path}[${n}]`, `[${n}]`, v, inner);
                    n++;
                }
            };

            details.onclick = (e) => {
                if (e.target !== summary) {
                    return;
                }

                if (details.open) {
                    inner.innerHTML = '';
                    open.delete(path);
                    return;
                }
                
                openDetails();
            };

            if (open.has(path)) {
                openDetails();
            }

            details.style.paddingLeft = `${pad}em`;
        } else if (typeof tree === 'string' || typeof tree === 'bigint' || typeof tree === 'number') {
            const text = document.createElement('span');
            elem.appendChild(text);

            text.innerText = `${name} = ${tree}`;
            text.style.paddingLeft = `${pad}em`;
        } else if (tree instanceof Fun) {
            const text = document.createElement('span');
            elem.appendChild(text);
            
            text.innerText = `${name} = <minivm code #${tree.n}>`;
            text.style.paddingLeft = `${pad}em`;
        } else if (tree instanceof FFI) {
            const text = document.createElement('span');
            elem.appendChild(text);
            
            text.innerText = `${name} = <native code #${tree.n}>`;
            text.style.paddingLeft = `${pad}em`;
        } else {
            console.log(tree);
        }
    };

    body.innerHTML = '';
    body.style.fontFamily = 'monospace';
    more('_ENV', '_ENV', tree, body);

    return body;
}
