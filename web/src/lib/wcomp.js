
import { comp } from './comp.js';

self.onmessage = async ({data}) => {
    switch (data.type) {
        case 'comp': {
            const output = await comp(data.input);
            self.postMessage({
                type: 'result',
                output: output,
                number: data.number,
            });
        }
    }
};

self.postMessage({type: 'ready'});
