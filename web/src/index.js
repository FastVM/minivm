
import Module from '../../build/bin/minivm.js';
import Emception from "./emception.js";

const main = async () => {
    const emception = new Emception();
    window.emception = emception;

    emception.onstdout = (str) => console.log(str);
    emception.onstderr = (str) => console.error(str);

    await emception.init();
    window.vm_compile_c_to_wasm_is_done = Array(256).fill(false);

    emception.run('emcc --version');

    const run = async (src) => {

        const mod = {
            arguments: ['--echo', '-e', src],
        };

        window.vm_compile_c_to_wasm = (n) => {
            window.vm_compile_c_to_wasm_is_done[n] = false;
            const cBuf = new TextDecoder().decode(mod.FS.readFile(`/in${n}.c`));
            // console.log(cBuf);
            emception.fileSystem.writeFile(`/working/in${n}.c`, cBuf);
            // const result = emception.run(`emcc -O2 -s SINGLE_FILE=1 -s SIDE_MODULE=1 -shared /working/in.c -o /working/out.wasm`);
            // console.log(v2.stdout);
            const result = emception.run(`emcc -O3 -s EXPORT_ALL=1 -s SIDE_MODULE=1 /working/in${n}.c -o /working/out${n}.wasm -Wno-version-check -Wno-incompatible-library-redeclaration`);
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
            const soBuf = emception.fileSystem.readFile(`/working/out${n}.wasm`);
            console.log(soBuf);
            mod.FS.writeFile(`/out${n}.so`, soBuf);
            window.vm_compile_c_to_wasm_is_done[n] = true;
        };

        await Module(mod);
    };

    window.run = run;

    console.log('installed: run()');
};

main();
