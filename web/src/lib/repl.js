
let comp;

const thens = [];

const waitForComp = () => {
    comp = new Worker(new URL(/* webpackChunkName: "wcomp" */ '../lib/wcomp.js', import.meta.url), {type: 'module'});
    return new Promise((ok, err) => {
        comp.onmessage = ({data}) => {
            switch (data.type) {
                case 'result': {
                    if (data.number != null) {
                        thens[data.number](data.output);
                    }
                    break;
                }
                case 'ready': {
                    ok();
                    break;
                }
            }
        };
    });
};

const unmap = (c) => {
    if (c === '\n') {
        return '\r\n';
    } else {
        return c;
    }
};

const wait = (ms) => new Promise((res, rej) => {
    setTimeout(res, ms);
});

const waitAsync = async (array, index, value) => {
    if (Atomics.waitAsync != null) {
        await Atomics.waitAsync(array, index, value);
    } else {
        let time = 0;
        while (true) {
            const got = Atomics.wait(array, index, value, 0);
            if (got !== 'timed-out') {
                return got; 
            }
            if (time < 20) {
                time += 1;
            }
            await wait(time);
        }
    }
};

export const repl = ({putchar}) => {
    const hasComp = waitForComp();
    const obj = {};
    obj.putchar = putchar;
    const has = new SharedArrayBuffer(4);
    const want = new SharedArrayBuffer(4);
    const inbuf = new SharedArrayBuffer(4);
    obj.input = async (str) => {
        const inbuf32 = new Int32Array(inbuf);
        const want32 = new Int32Array(want);
        const has32 = new Int32Array(has);
        for (const c of str) {
            await Atomics.waitAsync(want32, 0, 0).value;
            want32[0] = 0;
            inbuf32[0] = typeof c === 'string' ? c.charCodeAt(0) : c;
            has32[0] = 0
            Atomics.notify(has32, 0, 1);
        }
    };
    obj.start = async() => {
        const worker = new Worker(new URL(/* webpackChunkName: "wlua" */ '../lib/wlua.js', import.meta.url), {type: 'module'});
        const wait = new SharedArrayBuffer(4);
        const ret = new SharedArrayBuffer(65536);
        const number = thens.length;
        thens.push((buf) => {
            const len = buf.byteLength;
            new Uint8Array(ret).set(new Uint8Array(buf));
            const w32 = new Int32Array(wait);
            w32[0] = len
            Atomics.notify(w32, 0, 1);
        });
        worker.onmessage = async ({data}) => {
            switch (data.type) {
                case 'stdout': {
                    obj.putchar(unmap(data.stdout));
                    break;
                }
                case 'stderr': {
                    obj.putchar(unmap(data.stdout));
                    break;
                }
                case 'exit-err': {
                    err();
                    break;
                }
                case 'exit-ok': {
                    ok();
                    break;
                }
                case 'comp': {
                    await hasComp;
                    comp.postMessage({
                        type: 'comp',
                        number: number,
                        input: data.input,
                    });
                    break;
                }
                case 'get-buffer': {
                    worker.postMessage({
                        type: 'buffer',
                        ret: ret,
                        wait: wait,
                        inbuf: inbuf,
                        want: want,
                        has: has,
                    });
                    break;
                }
                case 'get-args': {
                    worker.postMessage({
                        type: 'args',
                        args: ['--repl'],
                    });
                    break;
                }
            }
        };
    };
    return obj;
};