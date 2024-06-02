
import { default as MiniVM } from '../build/bin/minivm.mjs';
import { default as BoxModule } from '../vendor/llvm-box/bin/llvm-box.mjs';
import { openSync, readSync } from 'fs';

const unlinkSync = (path) => {};

const mkdirSync = (path) => {
    box.FS.mkdir(path);
};

const writeFileSync = (path, data) => {
    box.FS.writeFile(path, data);
};

const readFileSync = (path) => {
    return box.FS.readFile(path);
};

const boxSpawn = (cmd, ...cmdArgs) => {
    const args = [...cmdArgs];
    switch (cmd) {
        case 'wasm-ld':
            args.unshift('lld', 'wasm-ld');
            break;
        case 'clang':
            args.unshift('clang', 'clang');
            break;
        default: 
            throw new Error(`command: ${cmd}`);
    }
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

const execSync = (str) => {
    try {
        boxSpawn(...str.split(/ +/));
    } catch (e) {
        if (!('status' in e) && e.status !== 0) {
            throw e;            
        }
    }
};

const cc = process.env['VM_CC'] ?? 'clang';
const ld = process.env['VM_LD'] ?? 'wasm-ld';

const join = (c, ld) => {
    const ldj = ld.split(' ').map(x => `-Wl,${x}`).join(' ');
    return `${c} ${ldj}`;
}

let ldFlagsBase = '-O0 --import-memory --strip-debug --export-dynamic --experimental-pic -shared';
let cFlagsBase = '-O2 -nostdlib -target wasm32-unknown-emscripten -fPIC -fvisibility=default';
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
            readSync(openSync('/dev/stdin'), buf, 0, 1, null);
            break;
        } catch (e) {
            console.error(e);
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

timeBegin('llvm');

const box = await BoxModule({
    noInitialRun: true,
    
    stdout(c) {
        process.stdout.write(new Uint8Array([c]));
    },
    
    stderr(c) {
        process.stderr.write(new Uint8Array([c]));
    },
});

const cache = '/work';
mkdirSync(cache);

timeEnd('llvm');

export const run = async (args) => {
    timeBegin('run');
    process.stdin.setRawMode(true);

    let last = 'mod';

    timeBegin(last);

    const mod = await MiniVM({
        noInitialRun: true,
        stdin: watch(stdinFunc),
        stdout: watch(stdoutFunc),
        stderr: watch(stderrFunc),

        _vm_lang_lua_repl_sync() {},

        _vm_compile_c_to_wasm: (n) => {
            timeEnd(last);
            const cSrc = mod.FS.readFile(`/in${n}.c`);
            let wasm = null;
            const inFile = `${cache}/in${n}.c`;
            const outFile = `${cache}/out${n}.wasm`;
            writeFileSync(inFile, cSrc);
            if (ld === 'none') {
                timeBegin(cc);
                execSync(`${cc} ${inFile} ${cFlags} -o ${outFile}`);
                timeEnd(cc);
                unlinkSync(inFile);
            } else {
                const midFile = `${cache}/mid${n}.o`;
                timeBegin(cc);
                execSync(`${cc} -c -w ${inFile} ${cFlagsBase} -o ${midFile}`);
                timeEnd(cc);
                unlinkSync(inFile);
                timeBegin(ld);
                execSync(`${ld} --no-entry --whole-archive ${midFile} ${ldFlagsBase} -o ${outFile}`);
                timeEnd(ld);
                unlinkSync(midFile);
            }
            wasm = readFileSync(outFile);
            unlinkSync(outFile);
            mod.FS.writeFile(`/out${n}.so`, wasm);
            last = `after${n}`;
            timeBegin(last);
        },
    });

    mod.FS.mkdir('/dir');
    mod.FS.mount(mod.NODEFS, {root : '.'}, '/dir');
    mod.FS.chdir('/dir');

    timeEnd(last);
    last = 'main';
    timeBegin(last);

    mod.callMain(args);

    timeEnd(last);

    process.stdin.setRawMode(false);

    timeEnd('run');
};

try {
    await run(process.argv.slice(2));
} catch (e) {
    console.error(e);
}
