<script>
    import '@xterm/xterm/css/xterm.css';

	import { repl } from '../lib/repl.js';
    import { onMount } from 'svelte';
    import { Terminal } from '@xterm/xterm';
    import { FitAddon } from '@xterm/addon-fit';

    let div;

    const term = new Terminal();

    const fit = new FitAddon();

    term.loadAddon(fit);

    onMount(() => {
        fit.fit();

        term.open(div);

        term.onData((data) => {
            obj.chars(data);
        });
    });

    const obj = repl({
        putchar: (str) => {
            term.write(str);
        }
    });

    obj.start();

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