import Emception from "./emception.js";

const emception = new Emception();

await emception.init();

emception.onstdout = (s) => console.log(s);
emception.onstderr = (s) => console.error(s);

// emception.run('emcc --check -Wno-version-check');

let flags1 = '-target wasm32-unknown-emscripten -fignore-exceptions -fPIC -fvisibility=default -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr -DEMSCRIPTEN -Wno-incompatible-library-redeclaration -Wno-parentheses-equality';
let flags2 = '-L/lazy/emscripten/cache/sysroot/lib/wasm32-emscripten/pic --no-whole-archive -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr --import-undefined --import-memory --strip-debug --export-dynamic --export=__wasm_call_ctors --experimental-pic -shared';

let comps = 0;
export const comp = (cBuf) => {
    comps += 1;
    emception.fileSystem.writeFile(`/working/in${comps}.c`, cBuf);
    const result1 = emception.runx(`/usr/bin/clang -O1 -c ${flags1} /working/in${comps}.c -o /working/mid${comps}.o`);
    if (result1.returncode !== 0) {
        console.error(`clang exited with code ${result1.returncode}`);
    }
    const result2 = emception.runx(`/usr/bin/wasm-ld --whole-archive /working/mid${comps}.o ${flags2} -o /working/out${comps}.wasm`);
    if (result2.returncode !== 0) {
        console.error(`wasm-ld exited with code ${result2.returncode}`)
    }
    return emception.fileSystem.readFile(`/working/out${comps}.wasm`);
};

// comp('int main() {}');
