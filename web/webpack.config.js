import CompressionPlugin from 'compression-webpack-plugin';
import CopyPlugin from 'copy-webpack-plugin';
import MiniCssExtractPlugin from 'mini-css-extract-plugin';
import path from 'path';

const dev = false;

export default {
    mode: dev ? 'development' : 'production',
    entry: {
        'minivm': './src/index.js',
    },
    output: {
        clean: true,
        path: path.resolve('public'),
        filename: '[name].js',
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
            "vm": false,
            fs: false,
            process: false,
        },
    },
    module: {
        rules: [
            {
                test: /\.svelte$/,
                use: {
                    loader: 'svelte-loader',
                    options: {
                        compilerOptions: {
                            dev: dev,
                        },
                        hotReload: dev,
                    },
                },
            },
			{
				test: /\.(css|style)$/,
				use: [
					MiniCssExtractPlugin.loader,
                    'css-loader',
				]
			},
            {
                test: /\.wasm$/,
                type: "asset/resource",
            },
            {
                test: /\.(pack|br|a)$/,
                type: "asset/resource",
                generator: {
                    filename: 'res/[name][ext]',
                },
            },
            {
                test: /\.m?js$/,
                use: {
                    loader: 'babel-loader',
                    options: {
                        presets: [
                            [
                                '@babel/preset-env',
                            ],
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
        new MiniCssExtractPlugin({
            filename: '[name].css',
        }),
        new CopyPlugin({
            patterns: [
                { from: "src/index.html", to: "index.html" },
            ],
        }),
        new CompressionPlugin({
            exclude: /\.br$/,
        }),
    ],
    devServer: {
        headers: {
            "Cross-Origin-Embedder-Policy": "require-corp",
            "Cross-Origin-Opener-Policy": "same-origin",
        },
        client: {
            logging: 'warn',
        },
    },
    experiments: {
        topLevelAwait: true,
    },
};