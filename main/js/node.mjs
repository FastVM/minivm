#!/usr/bin/env node

import {run, config} from './all.mjs';

run(process.argv.slice(2), config);
