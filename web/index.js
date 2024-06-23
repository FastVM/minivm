
import Module from './minivm.js';

const canvas = document.createElement('canvas');

document.body.appendChild(canvas);

canvas.id = 'canvas';

const lastGame = localStorage.getItem('minivm.game');

const app = new URLSearchParams(document.location.search).get('app') ?? lastGame;

localStorage.setItem('minivm.game', app);

const path = `minivm.save:${app}`;

const mod = await Module({
    arguments: [`test/app/${app}.lua`],
    canvas: canvas,
    preRun: (mod) => {
        if (lastGame !== app) {
            return;
        }
        const s = localStorage.getItem(path);
        if (s == null) {
            return;
        }
        const a = Uint8Array.from(JSON.parse(s));
        mod.FS.writeFile('/in.bin', a);
    },
    _vm_repl_sync: () => {
        const s = mod.FS.readFile('/out.bin');
        if (s == null) {
            return;
        }
        localStorage.setItem(path, JSON.stringify(Array.from(s)));
    },
});