
// import {run} from './lib/spawn.js';

// window.lua = async (src) => {
//     const {stdout, stderr} = await run(src);
//     stdout;
// };

import './app/global.css';
import App from './app/App.svelte';

new App({
    target: document.body,
});
