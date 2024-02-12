<script lang="js">
	import 'xterm/css/xterm.css';
	import { onMount } from 'svelte';
	import * as xterm from 'xterm';
	import * as fit from 'xterm-addon-fit';
	import { Readline } from 'xterm-readline';
	import parse from 'bash-parser';
	
	let terminalElement;
	let terminalController;
	let termFit;
	
	const thens = [];
	const comp = new Worker(new URL('../lib/wcomp.js', import.meta.url));
	
	const lua = (args) => new Promise((ok, err) => {
		const wait = new SharedArrayBuffer(4);
		const ret = new SharedArrayBuffer(65536);
		const worker = new Worker(new URL('../lib/wlua.js', import.meta.url));
		const number = thens.length;
		thens.push((buf) => {
			const len = buf.byteLength;
			new Uint8Array(ret).set(new Uint8Array(buf));
			const w32 = new Int32Array(wait);
			w32[0] = len
			Atomics.notify(w32, 0, 1);
		});
		worker.onmessage = async ({data}) => {
			switch (data.type) {
				case 'stdout': {
					rl.println(data.stdout);
					break;
				}
				case 'stderr': {
					rl.println(data.stdout);
					break;
				}
				case 'exit-err': {
					err();
					break;
				}
				case 'exit-ok': {
					ok();
					break;
				}
				case 'comp': {
					comp.postMessage({
						type: 'comp',
						number: number,
						input: data.input,
					});
					break;
				}
				case 'get-buffer': {
					worker.postMessage({
						type: 'buffer',
						ret: ret,
						wait: wait,
					});
					break;
				}
				case 'get-args': {
					worker.postMessage({
						type: 'args',
						args: args,
					});
					break;
				}
			}
		};
	});

	const loop = async (rl) => {
		while (true) {
			const res = await rl.read('$ ');
			for (const {type, name, suffix} of parse(res).commands) {
				if (type === 'SimpleCommand') {
					switch (name) {
						case 'lua': {
							const args = [];
							for (const obj of suffix) {
								if (obj.type == 'Word') {
									args.push(obj.text);
								} else {
									console.log(obj);
								}
							}
							await lua(args);
							break;
						}
					}
				}
			}
		}
	};

	const initalizeXterm = async() => {
		const rl = new Readline();
		
		terminalController = new xterm.Terminal();
		termFit = new fit.FitAddon();
		terminalController.loadAddon(rl);
		terminalController.loadAddon(termFit);
		terminalController.open(terminalElement);
		termFit.fit();

		await new Promise((ok, err) => {
			comp.onmessage = ({data}) => {
				switch (data.type) {
					case 'result': {
						thens[data.number](data.output);
						break;
					}
					case 'ready': {
						ok();
						break;
					}
				}
			};
		});
	
		loop(rl);
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

