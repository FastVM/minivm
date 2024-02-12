
import {run} from './lua.js';
import {comp} from './comp.js';

const stdin = () => {
    return null;
};

let buf = '';
const stdout = (str) => {
    if (str == '\n') {
        console.log(buf);
        buf = '';
    } else {
        buf += str;
    }
};

const stderr = (str) => {
    if (str == '\n') {
        console.log(buf);
        buf = '';
    } else {
        buf += str;
    }
};

export const lua = (lua) => {
    return run(['-e', lua], {stdin, stdout, stderr, comp});
}
