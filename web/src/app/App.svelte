
<script lang="ts">
	import 'xterm/css/xterm.css';
	import { onMount } from 'svelte';
	import * as xterm from 'xterm';
	import * as fit from 'xterm-addon-fit';
	
	let terminalElement;
	let terminalController;
	let termFit;
	$: {
		if (terminalController) {
			// ...
		}
	}
	const initalizeXterm = () => {
		terminalController = new xterm.Terminal();
		termFit = new fit.FitAddon();
		terminalController.loadAddon(termFit);
		terminalController.open(terminalElement);
		termFit.fit();
		terminalController.write('I am a terminal!');
		terminalController.onData((e) => {
			console.log(e);
		})
	};

	onMount(async () => {
		initalizeXterm();
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

