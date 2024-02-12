<script>
    import { onMount } from "svelte";
	import { repl } from '../lib/repl.js';

    let data = '';
    let src = '';
    
    const obj = repl({
        putchar: (str) => {
            data += str;
        }
    });

    const key = (event) => {
        if (event.key === 'Enter') {
            obj.input(src + '\n');
            src = '';
        }
    };

    onMount(() => {
        obj.start();
    });
</script>

<style>
    input {
        width: 100%;
        font-family: monospace;
        padding: 1em;
        border: 0;
        border-color: transparent;
        outline: none;
        background-color: black;
        color: greenyellow;
        user-select: none;
    }
   
    textarea {
        width: 100%;
        height: 100%;
        resize: none;
        background-color: black;
        color: greenyellow;
        border: 0;
        padding: 1em;
        padding-top: 0em;
        outline: none;
        height: min-content;
        flex-grow: 1;
    }

    .term {
        display: flex;
        flex-direction: column;
        background-color: #111;
        width: 100%;
        padding: 0em;
        border: 0em;
        height: 100%;
    }
</style>

<div class="term">
    <input type="text" on:keypress={key} bind:value={src}/>
    <textarea readonly>{data}</textarea>
</div>