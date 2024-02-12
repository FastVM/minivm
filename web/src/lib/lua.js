
import Module from '../../../build/bin/minivm.js';
import wasmBinary from '../../../build/bin/minivm.wasm';

const wasmBuffer = await (await fetch(wasmBinary)).arrayBuffer();

export const run = (args, opts) => {
    const stdinFunc = () => {
        return opts.stdin();
    };

    const stdoutFunc = (c) => {
        opts.stdout(String.fromCharCode(c));
    };

    const stderrFunc = (c) => {
        opts.stderr(String.fromCharCode(c));
    };

    const comp = (n) => {
        const cBuf = new TextDecoder().decode(mod.FS.readFile(`/in${n}.c`));
        const soBuf = opts.comp(cBuf);
        mod.FS.writeFile(`/out${n}.so`, soBuf);
    };

    globalThis.vm_compile_c_to_wasm = comp;
    
    const mod = Module({
        noInitialRun: true,
        wasmBinary: wasmBuffer,
        preRun: (mod) => {
            mod.FS.init(stdinFunc, stdoutFunc, stderrFunc);
        },
    });

    mod.callMain(args);
};
