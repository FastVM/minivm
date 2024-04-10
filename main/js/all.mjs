
import Module from '../../build/bin/minivm.mjs';
import { readFileSync, writeFileSync, existsSync, mkdirSync, unlinkSync, readSync } from 'node:fs';
import { execSync } from 'node:child_process';

if (!existsSync('.minivm-cache')) {
    mkdirSync('.minivm-cache');
}

const cc = `clang`;
const ld = `wasm-ld`;

let cFlags = '-c -target wasm32-unknown-emscripten -fignore-exceptions -fPIC -fvisibility=default -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr -DEMSCRIPTEN -Wno-incompatible-library-redeclaration -Wno-parentheses-equality';
let ldFlags = '-L/lazy/emscripten/cache/sysroot/lib/wasm32-emscripten/pic --no-whole-archive -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr --import-undefined --import-memory --strip-debug --export-dynamic --export=__wasm_call_ctors --experimental-pic -shared';

const watch = (f) => {
    return (...args) => {
        // console.log('(', f.name, args, ')');
        const got = f(...args);
        // console.log(' => ', got);
        return got;
    }
};

const stdinFunc = () => {
    const buf = Buffer.alloc(1);
    while (true) {
        try {
            readSync(process.stdin.fd, buf, 0, 1, null);
            break;
        } catch (e) {
            continue;
        }
    }
    const u8 = new Uint8Array(buf)[0];
    return u8;
};

const stdoutFunc = (c) => {
    process.stdout.write(new Uint8Array([c]));
};

const stderrFunc = (c) => {
    process.stderr.write(new Uint8Array([c]));
};

export const run = (args) => {
    process.stdin.setRawMode(true);

    const mod = Module({
        noInitialRun: true,
        stdin: watch(stdinFunc),
        stdout: watch(stdoutFunc),
        stderr: watch(stderrFunc),
        ENV: process.env,
        _vm_compile_c_to_wasm: (n) => {
            const inFile = `.minivm-cache/in${n}.c`;
            const midFile = `.minivm-cache/mid${n}.o`;
            const outFile = `.minivm-cache/out${n}.wasm`;
            const cSrc = mod.FS.readFile(`/in${n}.c`);
            writeFileSync(inFile, cSrc);
            execSync(`${cc} -O2 ${inFile} ${cFlags} -o ${midFile}`);
            execSync(`${ld} --whole-archive ${midFile} ${ldFlags} -o ${outFile}`)
            const wasm = readFileSync(outFile);
            unlinkSync(inFile);
            unlinkSync(midFile);
            unlinkSync(outFile);
            mod.FS.writeFile(`/out${n}.so`, wasm);
        },
    });

    mod.callMain(args);
  
    process.stdin.setRawMode(false);
};
