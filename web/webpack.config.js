import CompressionPlugin from 'compression-webpack-plugin';
import CopyPlugin from 'copy-webpack-plugin';
import path from 'path';

export default {
    mode: 'development',
    entry: './src/index.js',
    output: {
        clean: true,
        path: path.resolve('public'),
        filename: 'index.js',
    },
    resolve: {
        modules: [path.resolve('..', '..', 'node_modules'), 'node_modules'],
        fallback: {
            "llvm-box.wasm": false,
            "binaryen-box.wasm": false,
            "python.wasm": false,
            "quicknode.wasm": false,
            "path": false,
            "node-fetch": false,
            "vm": false
        },
    },
    module: {
        rules: [
            {
                test: /\.wasm$/,
                type: "asset/resource",
            },
            {
                test: /\.(pack|br|a)$/,
                type: "asset/resource",
            },
            {
                test: /\.m?js$/,
                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: [
                            [
                                '@babel/preset-env',
                                {
                                    targets: "chrome 60"
                                },
                            ],
                            // [
                            //     'minify',
                            //     {
                            //         builtIns: false,
                            //         // evaluate: false,
                            //         mangle: false,
                            //     },
                            // ],
                        ],
                    },
                },
            },
        ],
    },
    performance: {
        hints: false,
        maxEntrypointSize: 512000,
        maxAssetSize: 512000
    },
    plugins: [
        new CopyPlugin({
            patterns: [
                { from: "src/index.html", to: "index.html" },
            ],
        }),
        // new CompressionPlugin({
        //     exclude: /\.br$/,
        // }),
    ],
    devServer: {
        headers: {
            "Cross-Origin-Embedder-Policy": "require-corp",
            "Cross-Origin-Opener-Policy": "same-origin",
        }
    },
};