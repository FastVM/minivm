
import {run} from './lua.js';

let inbuf;
const stdin = () => {
    const i32 = new Int32Array(inbuf);
    i32[0] = 0;
    Atomics.wait(i32, 0, 0);
    console.log(i32[0]);
    Atomics.notify(i32, 0, 1);
    return i32[0];
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
