/**
 * @license
 * Copyright 2013 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

mergeInto(LibraryManager.library, {
    $FSROOT__deps: ["$FS"],
    $FSROOT__postset: "FSROOT.staticInit();",
    $FSROOT: {
        staticInit: () => {
            FS.root = null;

            let opts = (Module.ROOT && Module.ROOT.opts) || {};
            let type = (Module.ROOT && Module.ROOT.type) || "MEMFS";
            if (typeof type === "string") {
                type = FS.filesystems[type] || eval(type);
            } else if (typeof type === "function") {
                type = type(Module);
            }
            FS.mount(type, opts, '/');

            FSROOT.createDefaultMountPoints();

            // We need to ignore errors in mkdir
            // since we are pre-creation mountpoints
            // for directories otherwise created by the
            // FS.create* functions
            const restore_mkdir = FSROOT.safeMkdir();

            FS.createDefaultDirectories();
            FS.createDefaultDevices();
            FS.createSpecialDirectories();

            restore_mkdir();
        },
        createDefaultMountPoints: () => {
            // Mount a new MEMFS for /dev
            FS.mkdirTree("/dev");
            FS.mount(MEMFS, {}, "/dev");

            // Mount a new MEMFS for /proc/self
            FS.mkdirTree('/proc/self');
            FS.mount(MEMFS, {}, '/proc/self');
        },
        safeMkdir: () => {
            const mkdir = FS.mkdir;
            FS.mkdir = (path, mode) => {
                try {
                    return mkdir(path, mode);
                } catch {
                    // ignore errors
                    return FS.lookupPath(path, { follow: true }).node;
                }
            };
            return () => {
                FS.mkdir = mkdir;
            };
        },
    },
});

DEFAULT_LIBRARY_FUNCS_TO_INCLUDE.push('$FSROOT');
