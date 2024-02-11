
import { comp } from './comp.js';

self.onmessage = ({data}) => {
    switch (data.type) {
        case 'comp': {
            const output = comp(data.input);
            self.postMessage({
                type: 'result',
                output: output,
                number: data.number,
            });
        }
    }
};

self.postMessage({type: 'ready'});
