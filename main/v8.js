#!/usr/bin/env v8

let n = 1;

const wasm = readbuffer(arguments[2] ?? __VM_WASM_FILE_PATH__);

const buf = new Uint8Array(readbuffer(arguments[1] ?? "out.bc"));

const putchar = (c) => {
    write(String.fromCharCode(c));
};

const no_such_thing = () => {
    throw new Error("this should not happen");
};

const res = WebAssembly.instantiate(wasm, {
    wasi_unstable: {
        fd_write: putchar,
        fd_read: no_such_thing
    }
})
    .then(obj => {
        obj.instance.exports.vm_xset_putchar();
        for (const op of buf) {
            obj.instance.exports.vm_xadd(op);
        }
        obj.instance.exports.vm_xrun();
    })
    .catch(err => {
        print(err);
    });
