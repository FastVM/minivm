
Promise.all([
    import('./minivm.js'),
    import('./inspect.js'),
    fetch('./minivm.wasm'),
]).then(async ([{default: Module}, inspect, wasm]) => {
    const saveInterval = 1;

    const wasmBinary = await wasm.arrayBuffer();
    const canvas = document.createElement('canvas');
    document.body.appendChild(canvas);

    const lastGame = localStorage.getItem('minivm.game');
    const app = new URLSearchParams(document.location.search).get('app') ?? lastGame ?? 'snake';
    localStorage.setItem('minivm.game', app);
    const path = `minivm.save:${app}`;

    let lastKeys = new Set();
    let keys = new Set();
    let buttons = new Set();
    let lastButtons = new Set();
    let ptrx = 0;
    let ptry = 0;

    document.addEventListener('keydown', ({ key }) => {
        if (key.length !== 1) {
            return;
        }
        keys.add(key.toUpperCase().charCodeAt(0));
    });

    document.addEventListener('keyup', ({ key }) => {
        if (key.length !== 1) {
            return;
        }
        keys.delete(key.toUpperCase().charCodeAt(0));
    });

    document.addEventListener('mousedown', ({ button }) => {
        buttons.add(button);
    });

    document.addEventListener('mouseup', ({ button }) => {
        buttons.delete(button);
    });

    canvas.addEventListener('mousemove', ({ offsetX, offsetY }) => {
        ptrx = offsetX;
        ptry = offsetY;
    });

    let ctx2d = canvas.getContext('2d');

    ctx2d.fillStyle = '#FF00FF';
    ctx2d.fillRect(0, 0, 100, 100);

    const body = document.createElement('div');
    document.body.append(body);
    body.style.userSelect = 'none';
    
    const open = new Set(JSON.parse(localStorage.getItem('minivm.open') ?? '[]'));

    Module({
        arguments: [`test/app/${app}.lua`],
        wasmBinary: wasmBinary,
        preRun(mod) {
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
        _vm_canvas_draw_rect(x, y, w, h, r, g, b, a) {
            ctx2d.fillStyle = `rgba(${r}, ${g}, ${b}, ${a / 255})`;
            ctx2d.fillRect(x, y, Math.round(w + 1), Math.round(h + 1));
        },
        _vm_canvas_mouse_x() {
            return ptrx | 0;
        },
        _vm_canvas_mouse_y() {
            return ptry | 0;
        },
        _vm_canvas_mouse_pressed(button) {
            return buttons.has(button);
        },
        _vm_canvas_mouse_pressed_last(button) {
            return lastButtons.has(button);
        },
        _vm_canvas_key_pressed(key) {
            return keys.has(key);
        },
        _vm_canvas_key_pressed_last(key) {
            return lastKeys.has(key);
        },
        _vm_canvas_set_size(w, h) {
            if (w !== canvas.width || h !== canvas.height) {
                canvas.width = w;
                canvas.height = h;
                ctx2d = canvas.getContext('2d');
                ctx2d.fillStyle = '#FF00FF';
                ctx2d.fillRect(0, 0, w, h);
            }
        },
        _vm_std_app_frame_loop(vm) {
            let n = 0;
            const frame = () => {
                requestAnimationFrame(frame);
                const nKeys = new Set(keys);
                const nButtons = new Set(buttons);
                keys = new Set(keys);
                buttons = new Set(buttons);
                this._vm_std_app_frame(vm);
                lastButtons = nButtons;
                lastKeys = nKeys;
                if (n++ % saveInterval === 0) {
                    this._vm_std_app_sync(vm);
                    const s = this.FS.readFile('/out.bin');
                    if (s != null) {
                        localStorage.setItem(path, JSON.stringify(Array.from(s)));
                    }
                    body.onclick = () => {
                        const div = inspect.render(new Uint8Array(s), open);
                        body.innerHTML = '';
                        body.appendChild(div);
                        localStorage.setItem('minivm.open', JSON.stringify(Array.from(open)));
                    };
                    if (body.children.length === 0) {
                        body.onclick();
                    }
                }
            };
            requestAnimationFrame(frame);
        }
    });
});