
import {run} from './lua.js';

let want;
let inbuf;
const stdin = () => {
    const i32buf = new Int32Array(inbuf);
    const i32want = new Int32Array(want);
    i32want[0] = 1;
    Atomics.notify(i32want, 0, 1);
    Atomics.wait(i32buf, 0, i32buf[0]);
    return i32buf[0];
};

const stdout = (msg) => {
    self.postMessage({
        type: 'stdout',
        stdout: msg,
    });
};

const stderr = (msg) => {
    self.postMessage({
        type: 'stderr',
        stderr: msg,
    });
};

let wait;
let ret;
const comp = (input) => {
    self.postMessage({
        type: 'comp',
        input: input,
    });
    new Int32Array(wait)[0] = 0;
    Atomics.wait(new Int32Array(wait), 0, 0);
    const len = new Int32Array(wait)[0];
    const old = new Uint8Array(ret);
    const tmp = new Uint8Array(len);
    for (let i = 0; i < len; i++) {
        tmp[i] = old[i];
    }
    return tmp;
};

const onArgs = (args) => {
    run(args, {stdin, stdout, stderr, comp});
    self.postMessage({
        type: 'exit-ok',
    });
};

self.onmessage = ({data}) => {
    switch(data.type) {
        case 'buffer': {
            self.postMessage({type: 'get-args'});
            wait = data.wait;
            ret = data.ret;
            inbuf = data.inbuf;
            want = data.want;
            break;
        }
        case 'stdin': {
            
            break;
        }
        case 'args': {
            onArgs(data.args);
            break;
        }
    }
};

self.postMessage({type: 'get-buffer'});
