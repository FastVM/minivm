
import Module from '../../build/bin/minivm.mjs';
import { readFileSync, writeFileSync, existsSync, mkdirSync, readSync, unlinkSync } from 'node:fs';
import { execSync } from 'node:child_process';

if (!existsSync('.minivm-cache')) {
    mkdirSync('.minivm-cache');
}

const cc = `clang`;
const ld = `wasm-ld`;

const cFlagsList = [
    '-target wasm32-unknown-unknown',
    '-fignore-exceptions',
    '-fPIC',
    '-fvisibility=default',
];
const ldFlagsList = [
    '--no-whole-archive',
    '--import-undefined',
    '--import-memory',
    '--strip-debug',
    '--export-dynamic',
    '--experimental-pic',
    '-shared',
];

const cflags = cFlagsList.join(' ');
const ldflags = ldFlagsList.join(' ');

export const run = (args, opts) => {
    const stdinFunc = () => {
        return opts.stdin();
    };

    const stdoutFunc = (c) => {
        opts.stdout(String.fromCharCode(c));
    };

    const stderrFunc = (c) => {
        opts.stderr(String.fromCharCode(c));
    };
    
    const mod = Module({
        noInitialRun: true,
        preRun: (mod) => {
            mod.FS.init(stdinFunc, stdoutFunc, stderrFunc);
        },
        _vm_lang_lua_read: () => {
            return stdinFunc();
        },
        _vm_compile_c_to_wasm: (n) => {
            const inFile = `.minivm-cache/in${n}.c`;
            const midFile = `.minivm-cache/mid${n}.o`;
            const outFile = `.minivm-cache/out${n}.wasm`;
            const cSrc = mod.FS.readFile(`/in${n}.c`);
            writeFileSync(inFile, cSrc);
            execSync(`${cc} -O2 -c -w ${cflags} ${inFile} -o ${midFile}`);
            execSync(`${ld} ${ldflags} ${midFile} -o ${outFile}`)
            const wasm = readFileSync(outFile);
            unlinkSync(inFile);
            unlinkSync(midFile);
            unlinkSync(outFile);
            mod.FS.writeFile(`/out${n}.so`, wasm);
        },
    });

    mod.callMain(args);
};

export const config = {
    stdout(chr) {
        process.stdout.write(chr);
    },

    stderr(chr) {
        process.stderr.write(chr);
    },

    stdin() {
        const buf = Buffer.alloc(1);
        readSync(0, buf, 0, 1);
        return new Uint8Array(buf)[0];
    },
};

