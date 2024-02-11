
import {run} from './lib/lua.js';

const onArgs = (args) => {
    console.log(args);
    const {stdout, stderr} = run(...args);
    self.postMessage({
        type: 'stdout',
        stdout: stdout,
    });
    self.postMessage({
        type: 'stderr',
        stderr: stderr,
    });
    self.postMessage({
        type: 'exit-ok',
    });
};

self.ready

self.onmessage = ({data}) => {
    switch(data.type) {
        case 'args': {
            onArgs(data.args);
            break;
        }
    }
};

self.postMessage({type: 'ready'});
