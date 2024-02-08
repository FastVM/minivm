
import Module from '../../build/bin/minivm.js';
import Emception from "./emception.js";

const emception = new Emception();
window.emception = emception;

// emception.onstdout = (str) => console.log(str);
// emception.onstderr = (str) => console.error(str);

await emception.init();

const run = async (src) => {
    const mod = {
        arguments: ['--echo', '-e', src],
    };

    window.vm_compile_c_to_wasm = async () => {
        const cBuf = new TextDecoder().decode(mod.FS.readFile(`/in.c`));
        console.log(cBuf);
        await emception.fileSystem.writeFile("/working/in.c", cBuf);
        const result = await emception.run(`emcc -O2 -s SINGLE_FILE=1 -s SIDE_MODULE=1 -shared /working/in.c -o /working/out.wasm`);
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
        const soBuf = await emception.fileSystem.readFile('/working/out.wasm');
        // console.log(soBuf);
        await mod.FS.writeFile(`/out.so`, soBuf);
    };

    await Module(mod);
};

window.run = run;

console.log('installed: run()');
