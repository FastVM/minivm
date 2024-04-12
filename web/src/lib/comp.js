
import { default as BoxModule } from '../../../vendor/llvm-box/bin/llvm-box.mjs';

const box = await BoxModule();

let ldFlagsBase = '-O0 --import-memory --strip-debug --export-dynamic --experimental-pic -shared';
let cFlagsBase = '-O2 -nostdlib -target wasm32-unknown-emscripten -fPIC -fvisibility=default';

const boxSpawn = (real, cmd, ...cmdArgs) => {
    const args = [real, cmd, ...cmdArgs];
    const argc = args.length;
    const argv = box._malloc((argc + 1) * 4);
    let argv_ptr = argv;
    for (const arg of args) {
        const arr = new TextEncoder().encode(arg);
        const str = box._malloc(arr.length + 1);
        for (let i = 0; i < arr.length; i++) {
            box.HEAPU8[str + i] = arr[i];
        }
        box.HEAPU8[str + arr.length] = 0;
        box.HEAPU32[argv_ptr >> 2] = str;
        argv_ptr += 4
    };
    box.HEAPU32[argv_ptr >> 2] = 0;
    return box._main(argc, argv);
};

const execSync = (real, str) => {
    try {
        boxSpawn(real, ...str.split(/ +/));
    } catch (e) {
        if (!('status' in e) && e.status !== 0) {
            throw e;            
        }
    }
};

let comps = 0;
export const comp = (cSrc) => {
    const n = comps++;
    const inFile = `/in${n}.c`;
    const outFile = `/out${n}.wasm`;
    box.FS.writeFile(inFile, cSrc);
    const midFile = `/mid${n}.o`;
    execSync('clang', `clang -c -w ${inFile} ${cFlagsBase} -o ${midFile}`);
    execSync('lld', `wasm-ld --no-entry --whole-archive ${midFile} ${ldFlagsBase} -o ${outFile}`);
    return box.FS.readFile(outFile);
};

// comp('int main() {}');
