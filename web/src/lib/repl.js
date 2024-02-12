
const comp = new Worker(new URL('../lib/wcomp.js', import.meta.url));

const thens = [];

const waitForComp = await new Promise((ok, err) => {
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

const unmap = (c) => {
    if (c === '\n') {
        return '\r\n';
    } else {
        return c;
    }
};

export const repl = ({putchar}) => {
    const obj = {};
    obj.putchar = putchar;
    const want = new SharedArrayBuffer(4);
    const inbuf = new SharedArrayBuffer(4);
    obj.input = async (str) => {
        const i32buf = new Int32Array(inbuf);
        const i32want = new Int32Array(want);
        for (const c of str) {
            await Atomics.waitAsync(i32want, 0, 0).value;
            i32want[0] = 0;
            i32buf[0] = c.charCodeAt(0);
            Atomics.notify(i32buf, 0, 1);
        }
    };
    obj.start = async() => {
        const wait = new SharedArrayBuffer(4);
        const ret = new SharedArrayBuffer(65536);
        const worker = new Worker(new URL('../lib/wlua.js', import.meta.url));
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
                    await waitForComp;
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