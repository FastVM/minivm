
import Module from '../../../build/bin/minivm.mjs';

export const run = (args, opts) => {
    const mod = Module({
        noInitialRun: true,
        _vm_compile_c_to_wasm(n) {
            const cBuf = new TextDecoder().decode(mod.FS.readFile(`/in${n}.c`));
            const soBuf = opts.comp(cBuf);
            mod.FS.writeFile(`/out${n}.so`, soBuf);
        },
        stdin() {
            return opts.stdin();
        },
        stdout(c) {
            opts.stdout(String.fromCharCode(c));
        },
        stderr(c) {
            opts.stderr(String.fromCharCode(c));
        },
    });

    mod.callMain(args);
};
