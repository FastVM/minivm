#!/usr/bin/env node

const fs = require("fs");
const src = fs.readFileSync(process.argv[2]);
const buf = new Uint8Array(fs.readFileSync(process.argv[3]));

const putchar = (c) => {
    process.stdout.write(String.fromCharCode(c));
};

const no_such_thing = () => {
    throw new Error("this should not happen");
};

WebAssembly.instantiate(src, {
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
    });