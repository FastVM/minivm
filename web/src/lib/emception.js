import FileSystem from "emception/FileSystem.mjs";

import LlvmBoxProcess from "emception/LlvmBoxProcess.mjs";
import BinaryenBoxProcess from "emception/BinaryenBoxProcess.mjs";
import Python3Process from "emception/Python3Process.mjs";
import NodeProcess from "emception/QuickNodeProcess.mjs";

import packs from "emception/packs.mjs";

const tools_info = {
    "/usr/bin/clang":                    "llvm-box",
    "/usr/bin/clang++":                  "llvm-box",
    "/usr/bin/llc":                      "llvm-box",
    "/usr/bin/lld":                      "llvm-box",
    "/usr/bin/llvm-ar":                  "llvm-box",
    "/usr/bin/llvm-nm":                  "llvm-box",
    "/usr/bin/llvm-objcopy":             "llvm-box",
    "/usr/bin/wasm-ld":                  "llvm-box",
    "/usr/bin/node":                     "node",
    "/usr/bin/python":                   "python",
    "/usr/bin/wasm-as":                  "binaryen-box",
    "/usr/bin/wasm2js":                  "binaryen-box",
    "/usr/bin/wasm-ctor-eval":           "binaryen-box",
    "/usr/bin/wasm-emscripten-finalize": "binaryen-box",
    "/usr/bin/wasm-metadce":             "binaryen-box",
    "/usr/bin/wasm-opt":                 "binaryen-box",
    "/usr/bin/wasm-shell":               "binaryen-box",
};

// packages needed for the startup example
const preloads = [
    "cpython",
    "emscripten",
    "emscripten_node_modules",
    "emscripten_sysroot_lib_wasm32-emscripten_libGL.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libal.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libc++.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libc++abi.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libc.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libcompiler_rt.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libdlmalloc.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libhtml5.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libsockets.a",
    "emscripten_sysroot_lib_wasm32-emscripten_libstubs.a",
    "emscripten_system_include",
    "emscripten_system_include_SDL",
    "emscripten_system_include_compat",
    "emscripten_system_lib_compiler-rt_lib",
    "emscripten_system_lib_libcxx_include",
    "emscripten_third_party",
    "wasm"
];

class Emception {
    fileSystem = null;
    tools = {};

    async init() {
        const fileSystem = await new FileSystem();
        this.fileSystem = fileSystem;

        fileSystem.mkdirTree("/lazy");
        for (const [name, url] of Object.entries(packs)) {
            fileSystem.cachedLazyFolder(`/lazy/${name}`, url, 0o777, `/lazy/${name}`);
        }

        fileSystem.mkdirTree("/usr/local");
        fileSystem.symlink("/lazy/emscripten", "/emscripten");
        fileSystem.symlink("/lazy/cpython", "/usr/local/lib");
        fileSystem.symlink("/lazy/wasm", "/wasm");

        for (const preload of preloads) {
            await fileSystem.preloadLazy(`/lazy/${preload}`);
        }

        fileSystem.mkdirTree("/working");

        fileSystem.mkdirTree("/usr/bin");
        for (const tool of Object.keys(tools_info)) {
            // Emscripten checks the existence of a few of these files
            fileSystem.writeFile(tool, "");
        }

        const processConfig = {
            FS: fileSystem.FS,
            onrunprocess: (...args) => this._run_process(...args),
        };

        const tools = {
            "llvm-box": new LlvmBoxProcess(processConfig),
            "binaryen-box": new BinaryenBoxProcess(processConfig),
            "node": new NodeProcess(processConfig),
            "python": [
                new Python3Process(processConfig),
                new Python3Process(processConfig),
                new Python3Process(processConfig),
            ],
        };
        this.tools = tools;

        for (let tool in tools) {
            tools[tool] = await Promise.all([].concat(tools[tool]));
        }
    }

    onprocessstart = () => {};
    onprocessend = () => {};
    onstdout = () => {};
    onstderr = () => {};

    run(...args) {
        if (this.fileSystem.exists("/emscripten/cache/cache.lock")) {
            this.fileSystem.unlink("/emscripten/cache/cache.lock");
        }
        if (args.length == 1) args = args[0].split(/ +/);
        return this._run_process_impl([
            `/emscripten/${args[0]}.py`,
            ...args.slice(1)
        ], {
            print: (...args) => this.onstdout(...args),
            printErr: (...args) => this.onstderr(...args),
            cwd: "/working",
            path: ["/emscripten"],
        });
    };

    _run_process(argv, opts = {}) {
        this.onprocessstart(argv);
        const result = this._run_process_impl(argv, opts);
        this.onprocessend(result);
        return result;
    }

    _run_process_impl(argv, opts = {}) {
        const emscripten_script = argv[0].match(/^((\/lazy)?\/emscripten\/.+?)(?:\.py)?$/)?.[1]
        if (emscripten_script && this.fileSystem.exists(`${emscripten_script}.py`)) {
            argv = [
                "/usr/bin/python",
                // "-c","import os; os.environ['EMCC_SKIP_SANITY_CHECKS'] = '1'",
                // "-c","import os; os.environ['EMCC_FORCE_STDLIBS'] = '1'",
                "-E", `${emscripten_script}.py`,
                ...argv.slice(1)
            ];
        }
  
        const tool_name = tools_info[argv[0]];
        const tool = this.tools[tool_name]?.find(p => !p.running);
        if (!tool) {
            const result = {
                returncode: 1,
                stdout: "",
                stderr: `Emception tool not found: ${JSON.stringify(argv[0])}`,
            };
            return result;
        }
  
        const result = tool.exec(argv, {
            ...opts,
            cwd: opts.cwd || "/",
            path: ["/emscripten"]
        });

        this.fileSystem.push();
        return result;
    };
}

export default Emception;