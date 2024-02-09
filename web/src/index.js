
import Module from '../../build/bin/minivm.js';
import Emception from "./emception.js";

const main = async () => {
    const emception = new Emception();
    window.emception = emception;

    emception.onstdout = (str) => console.log(str);
    emception.onstderr = (str) => console.error(str);

    await emception.init();
    window.vm_compile_c_to_wasm_is_done = Array(256).fill(false);

    const run = async (src) => {

        let out = '';
        let err = '';

        const stdin = () => {
            return null;
        };
        const stdout = (...cs) => {
            for (const c of cs) {

                if (c == 10) {
                    console.log(out);
                    out = '';
                } else {
                    out += String.fromCharCode(c);
                }
            }
        };
        const stderr = (...cs) => {
            for (const c of cs) {
                if (c == 10) {
                    console.log(err);
                    err = '';
                } else {
                    err += String.fromCharCode(c);
                }
            }
        };

        const mod = {
            arguments: ['--echo', '-e', src],
            preRun: (mod) => {
                mod.FS.init(stdin, stdout, stderr);
            }
        };

        window.vm_compile_c_to_wasm =  (n) => {
            window.vm_compile_c_to_wasm_is_done[n] = false;
            const cBuf = new TextDecoder().decode(mod.FS.readFile(`/in${n}.c`));
            console.log(cBuf);
            emception.fileSystem.writeFile(`/working/in${n}.c`, cBuf);
            // const result = await emception.run(`emcc -O2 -s SINGLE_FILE=1 -s SIDE_MODULE=1 -shared /working/in.c -o /working/out.wasm`);
            emception.run(`emcc --version`);
            const result = emception.run(`emcc -g2 -shared /working/in${n}.c -o /working/out${n}.so -Wno-version-check -Wno-incompatible-library-redeclaration`);
            if (result.returncode !== 0) {
                console.error(`emcc exited with code ${result.returncode}`)
            }
            // const walk = (node) => {
            //     if (node.contents instanceof Uint8Array) {
            //         console.log(node);
            //     } else {
            //         console.log('folder', node);
            //         Object.values(node.contents).forEach(walk);
            //     }
            // }
            // walk(emception.fileSystem.FS.root);
            // console.log(result);
            // console.log(emception.fileSystem.FS.root.contents.working.contents);
            const soBuf = emception.fileSystem.readFile(`/working/out${n}.so`);
            // console.log(soBuf);
            mod.FS.writeFile(`/out${n}.so`, soBuf);
            window.vm_compile_c_to_wasm_is_done[n] = true;
        };

        await Module(mod);
    };

    window.run = run;

    console.log('installed: run()');
};

main();
