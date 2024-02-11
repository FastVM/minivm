
export const spawn = (...args) => new Promise((ok, err) => {
    const worker = new Worker(new URL('./worker.js', import.meta.url));
    let stdout = '';
    let stderr = '';
    worker.onmessage = ({data}) => {
        switch (data.type) {
            case 'stdout': {
                stdout += data.stdout;
                break;
            }
            case 'stderr': {
                stderr += data.stderr;
                break;
            }
            case 'exit-err': {
                err();
                break;
            }
            case 'exit-ok': {
                ok({ stdout, stderr });
                break;
            }
            case 'ready': {
                worker.postMessage({
                    type: 'args',
                    args: args,
                });
                break;
            }
        }
    };
});

export const run = (lua) => {
    return spawn('-e', lua);
}
