import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import wasm from 'vite-plugin-wasm';
import topLevelAwait from 'vite-plugin-top-level-await';

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [
        svelte(),
        wasm(),
        topLevelAwait(),
    ],
    base: './',
    worker: {
        format: 'es',
    },
    build: {
        outDir: 'public',
        rollupOptions: {
            output: {
                format: 'es',
            },
        },
    },
    assetsInclude: [
        '**/*.pack',
    ],
});
