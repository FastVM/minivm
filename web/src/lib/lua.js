
import Module from '../../../build/bin/minivm.js';
import wasmBinary from '../../../build/bin/minivm.wasm';

const wasmBuffer = await (await fetch(wasmBinary)).arrayBuffer();

export const run = (args, opts) => {
    const stdinFunc = () => {
        return null;
    };

    let stdout = '';
    const stdoutFunc = (c) => {
        if (c == 10) {
            opts.stdout(stdout);
            stdout = '';
        } else {
            stdout += String.fromCharCode(c);
        }
    };
    
    let stderr = '';
    const stderrFunc = (c) => {
        if (c == 10) {
            opts.stdout(stdout);
            stderr = '';
        } else {
            stderr += String.fromCharCode(c);
        }
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
