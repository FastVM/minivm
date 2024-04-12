#!/usr/bin/env node

import {run} from './all.js';

try {
    run(process.argv.slice(2));
} catch (e) {
    console.error(e);
}
