import Emception from "./emception.js";

const emception = new Emception();

await emception.init();

emception.run('emcc --check -Wno-version-check');

let comps = 0;
export const comp = (cBuf) => {
    comps += 1;
    emception.fileSystem.writeFile(`/working/in${comps}.c`, cBuf);
    const result = emception.run(`emcc -O3 -s EXPORT_ALL=1 -s SIDE_MODULE=1 /working/in${comps}.c -o /working/out${comps}.wasm -Wno-version-check -Wno-incompatible-library-redeclaration -Wno-parentheses-equality`);
    if (result.returncode !== 0) {
        console.error(`emcc exited with code ${result.returncode}`)
    }
    return emception.fileSystem.readFile(`/working/out${comps}.wasm`);
}
