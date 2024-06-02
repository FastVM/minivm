
import Module from '../../../build/bin/minivm.mjs';

export const run = (args, opts) => {
    const mod = Module({
        noInitialRun: true,
        _vm_compile_c_to_wasm(n) {
            mod.FS.writeFile(`/out${n}.so`, opts.comp(mod.FS.readFile(`/in${n}.c`)));
        },
        _vm_lang_lua_repl_sync() {
            const buf = mod.FS.readFile('/wasm.bin');
            self.postMessage({type: 'sync', buf: buf});
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

    if (opts.sync) {
        console.log(opts.sync);
        mod.FS.writeFile('/wasm.bin', new Uint8Array(opts.sync));
    }

    mod.callMain(args);
};
