<script lang="js">
	import 'xterm/css/xterm.css';
	import { onMount } from 'svelte';
	import * as xterm from 'xterm';
	import * as fit from 'xterm-addon-fit';
	import { repl } from '../lib/repl.js';

	let terminalElement;
	let terminalController;
	let termFit;


	onMount(async () => {
		terminalController = new xterm.Terminal();
		termFit = new fit.FitAddon();
		terminalController.loadAddon(termFit);
		terminalController.open(terminalElement);
		termFit.fit();
		const obj = repl({
			putchar: (str) => {
				terminalController.write(str);
			}
		});
		terminalController.onData((data) => {
			obj.input(data);
		});
		obj.start();
        setInterval(handleTermResize, 500);
	});
	
    const handleTermResize = () => {
		if (termFit) {
			termFit.fit();
		}
	};
</script>

<style>
    .terminal {
        height: 100%;
    }
</style>

<div 
	class="terminal"
	bind:this="{terminalElement}"
/>

