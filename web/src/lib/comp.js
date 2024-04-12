
import { default as BoxModule } from '../../../vendor/llvm-box/bin/llvm-box.mjs';

const bin = await (await fetch(new URL('../../../vendor/llvm-box/bin/llvm-box.wasm', import.meta.url))).arrayBuffer();

let ldFlagsBase = '-O0 --no-entry --import-memory --strip-debug --export-dynamic --experimental-pic -shared';
let cFlagsBase = '-O2 -nostdlib -target wasm32-unknown-emscripten -fPIC -fvisibility=default';

const exec = function(real, str) {
    try {
        const args = [real, ...str.split(/ +/)];
        const argc = args.length;
        const argv = this._malloc((argc + 1) * 4);
        let argv_ptr = argv;
        for (const arg of args) {
            const arr = new TextEncoder().encode(arg);
            const str = this._malloc(arr.length + 1);
            for (let i = 0; i < arr.length; i++) {
                this.HEAPU8[str + i] = arr[i];
            }
            this.HEAPU8[str + arr.length] = 0;
            this.HEAPU32[argv_ptr >> 2] = str;
            argv_ptr += 4
        };
        this.HEAPU32[argv_ptr >> 2] = 0;
        return this._main(argc, argv);
    } catch (e) {
        if (!('status' in e) && e.status !== 0) {
            throw e;            
        }
    }
};

const newModule = () => {
    return BoxModule({
        noInitialRun: true,
        wasmBinary: bin,
        exec: exec,
    });
};

const boxes = Array(4).fill(null).map(newModule);

let comps = 0;
export const comp = async (cSrc) => {
    const n = comps++ % boxes.length;

    const box = await boxes[n];

    boxes[n] = newModule();
    
    const inFile = `/in${n}.c`;
    const midFile = `/mid${n}.o`;
    const outFile = `/out${n}.wasm`;
    box.FS.writeFile(inFile, cSrc);
    box.exec('clang', `clang -c -w ${inFile} ${cFlagsBase} -o ${midFile}`);
    box.exec('lld', `wasm-ld --no-entry --whole-archive ${midFile} ${ldFlagsBase} -o ${outFile}`);
    const ret = box.FS.readFile(outFile);
    return ret;
};

// comp('int main() {}');
