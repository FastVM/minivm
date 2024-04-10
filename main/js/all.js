
import Module from '../../build/bin/minivm.mjs';
import { readFileSync, writeFileSync, existsSync, mkdirSync, unlinkSync, readSync } from 'node:fs';
import { execSync } from 'node:child_process';

if (!existsSync('.minivm-cache')) {
    mkdirSync('.minivm-cache');
}

const remove = (path) => {
    unlinkSync(path);
};

const cc = process.env['VM_CC'] ?? `clang`;
const ld = process.env['VM_LD'];

const join = (c, ld) => {
    const ldj = ld.split(' ').map(x => `-Wl,${x}`).join(' ');
    return `${c} ${ldj}`;
}

let ldFlagsBase = '-O0 -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr --import-undefined --import-memory --strip-debug --export-dynamic --export=__wasm_call_ctors --experimental-pic -shared';
let cFlagsBase = '-O2 -nostdlib -target wasm32-unknown-emscripten -fignore-exceptions -fPIC -fvisibility=default -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr -DEMSCRIPTEN -Wno-incompatible-library-redeclaration -Wno-parentheses-equality';
const cFlags = join(cFlagsBase, ldFlagsBase);

const time = Number(process.env['VM_TIME']);

const timeBegin = (name) => {
    if (time) {
        console.time(name);
    }
};

const timeEnd = (name) => {
    if (time) {
        console.timeEnd(name);
    }
};

const watch = (f) => {
    return (...args) => {
        try {
            // console.log('(', f.name, args, ')');
            const got = f(...args);
            // console.log(' => ', got);
            return got;
        } catch (e) {
            console.error(e);
            process.exit(1);
        }
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
    return new Uint8Array(buf)[0];
};

const stdoutFunc = (c) => {
    process.stdout.write(new Uint8Array([c]));
};

const stderrFunc = (c) => {
    process.stderr.write(new Uint8Array([c]));
};

export const run = (args) => {
    timeBegin('run');
    process.stdin.setRawMode(true);

    let last = 'mod';

    timeBegin(last);

    const mod = Module({
        noInitialRun: true,
        ENV: Object.create(process.env),
        stdin: watch(stdinFunc),
        stdout: watch(stdoutFunc),
        stderr: watch(stderrFunc),
        _vm_compile_c_to_wasm: (n) => {
            console.timeEnd(last);
            const inFile = `.minivm-cache/in${n}.c`;
            // const midFile = `.minivm-cache/mid${n}.o`;
            const outFile = `.minivm-cache/out${n}.wasm`;
            const cSrc = mod.FS.readFile(`/in${n}.c`);
            writeFileSync(inFile, cSrc);
            if (ld == null) {
                timeBegin(cc);
                execSync(`${cc} ${inFile} ${cFlags} -o ${outFile}`);
                timeEnd(cc);
            } else {
                const midFile = `.minivm-cache/mid${n}.o`;
                timeBegin(cc);
                execSync(`${cc} ${inFile} ${cFlags} -o ${midFile}`);
                timeEnd(cc);
                timeBegin(ld);
                execSync(`${ld} --whole-archive ${midFile} ${ldFlags} -o ${outFile}`)
                timeEnd(ld);
                remove(midFile);
            }
            const wasm = readFileSync(outFile);
            remove(inFile);
            remove(outFile);
            mod.FS.writeFile(`/out${n}.so`, wasm);
            last = `after${n}`;
            timeBegin(last);
        },
    });

    timeEnd(last);
    last = 'main';
    timeBegin(last);

    mod.callMain(args);

    timeEnd(last);
  
    process.stdin.setRawMode(false);
    
    timeEnd('run');
};
