<script>
    import '@xterm/xterm/css/xterm.css';

	import { repl } from '../lib/repl.js';
    import { onMount } from 'svelte';
    import { Terminal } from '@xterm/xterm';
    import { FitAddon } from '@xterm/addon-fit';
    import { openpty } from 'xterm-pty';

    let div;

    const term = new Terminal(new FitAddon());

    const fit = new FitAddon();

    term.loadAddon(fit);

    const { master, slave } = openpty();

    term.loadAddon(master);

    onMount(() => {
        fit.fit();

        term.open(div);
    });

    const obj = repl({
        putchar: (str) => {
            console.log(str);
            term.write(str);
        }
    });

    obj.start();

    slave.onReadable(() => {
        const src = slave.read();
        obj.chars(src);
    });

    const resize = () => {
        fit.fit();
    };
</script>

<style>
    .term {
        display: flex;
        flex-direction: column;
        background-color: #111;
        padding: 0em;
        border: 0em;
        width: 100%;
        height: 100%;
        overflow: auto;
    }
</style>

<div class="term" on:resize={resize} bind:this={div}/>