
import './app/global.css';
import App from './app/App.svelte';

new App({
    target: document.body,
});

// const fengari = (str) => {
//     load(str)();
// };
// window.fengari = fengari;

import {lua} from './lib/spawn.js';
const minivm = (str) => {
    lua(str);
};

window.minivm = minivm;
window.bench = (func, ...args) => {
    console.time(func.name);
    const ret = func(...args);
    console.timeEnd(func.name);
    return ret;
};
