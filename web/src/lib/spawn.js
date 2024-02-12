
import {run} from './lua.js';
import {comp} from './comp.js';

const stdin = () => {
    return null;
};

const stdout = (str) => {
    console.log(str);
};

const stderr = (str) => {
    console.error();(str);
};

export const lua = (lua) => {
    return run(['-e', lua], {stdin, stdout, stderr, comp});
}
