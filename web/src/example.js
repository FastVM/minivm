import Emception from "./emception.js";

const emception = new Emception();

if(!globalThis)
    var globalThis = {};

globalThis.emception = emception;

if(!window)
    var window = {};

window.emception = emception;
console.log("emception", emception);
console.log("window", window);

console.trace();

let cppValue = `
#include <stdio.h>

int main(void) {
    printf("Hello, World!\\n");
}
`;

// "-c","import os; os.environ['EMCC_SKIP_SANITY_CHECKS'] = '1'",

emception.onstdout = (str) => console.log(str);
emception.onstderr = (str) => console.error(str);

window.onerror = function(event) {
    // TODO: do not warn on ok events like simulating an infinite loop or exitStatus
    Module.setStatus('Exception thrown, see JavaScript console');
    Module.setStatus = function(text) {
        if (text) console.error('[post-exception status] ' + text);
    };
};

async function main() {
    const onprocessstart = (argv) => {
        console.log("onprocessstart", argv);
    };
    const onprocessend = () => {
        console.log("onprocessend");
    };

    emception.onprocessstart = onprocessstart;
    emception.onprocessend = onprocessend;

    console.log("Loading Emception...\n");

    await emception.init();

    console.log("Emception is ready\n");

    console.log("Compiling C++ code...\n");
    console.log(cppValue);

    try {
        await emception.fileSystem.writeFile("/working/main.cpp", cppValue);
        const cmd = `emcc -shared --no-entry -s SIDE_MODULE=1 -O2 -fexceptions -sEXIT_RUNTIME=1" -sSINGLE_FILE=1 -sUSE_CLOSURE_COMPILER=0 main.cpp -o main.js`;
        onprocessstart(`/emscripten/${cmd}`.split(/\s+/g));
        console.log(`$ ${cmd}\n\n`);
        const result = await emception.run(cmd);
        if (result.returncode == 0) {
            console.log("Emception compilation finished");
            const content = new TextDecoder().decode(await emception.fileSystem.readFile("/working/main.js"));
            eval(content);
            console.log("Execution finished");
        } else {
            console.log(`Emception compilation failed`);
        }
    } catch (err) {
        console.error(err);
    } finally {

    }
}

main();