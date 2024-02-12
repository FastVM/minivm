
import './app/global.css';
import {lua} from './lib/spawn.js';
import {load} from 'fengari-web';
import App from './app/App.svelte';

new App({
    target: document.body,
});

const fengari = (str) => {
    load(str)();
};

const minivm = (str) => {
    lua(str);
};

window.minivm = minivm;
window.fengari = fengari;
window.bench = (func, ...args) => {
    console.time(func.name);
    const ret = func(...args);
    console.timeEnd(func.name);
    return ret;
};
// console.log('window.{minvim, fengari, time} available');
