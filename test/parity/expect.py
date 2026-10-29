#!/usr/bin/env python3
import argparse
import os
import subprocess
import sys

base = os.path.dirname(os.path.abspath(__file__))
probes = os.path.join(base, "probes")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lua", default="lua5.5")
    ap.add_argument("names", nargs="+")
    args = ap.parse_args()
    lua = args.lua
    if os.path.sep in lua:
        lua = os.path.abspath(lua)
    for name in args.names:
        stem = name[:-4] if name.endswith(".lua") else name
        proc = subprocess.run([lua, stem + ".lua"], cwd=probes, capture_output=True, timeout=60)
        with open(os.path.join(probes, stem + ".expected"), "wb") as f:
            f.write(proc.stdout + proc.stderr)
        print(f"{stem}: {len(proc.stdout + proc.stderr)} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
