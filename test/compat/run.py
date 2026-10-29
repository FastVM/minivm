#!/usr/bin/env python3
import argparse
import os
import re
import subprocess
import sys

base = os.path.dirname(os.path.abspath(__file__))
root = os.path.normpath(os.path.join(base, "..", ".."))
minivm = os.path.join(root, "build", "bin", "minivm")
pointer = re.compile(rb"0x[0-9a-f]+")


def run(exe, script, timeout):
    try:
        proc = subprocess.run([exe, script], capture_output=True, timeout=timeout, cwd=root)
        out = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired:
        out = b"<timeout>"
    return pointer.sub(b"PTR", out).decode(errors="replace").split("\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lua", default="lua5.5")
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("files", nargs="*")
    args = ap.parse_args()
    names = args.files
    if not names:
        names = sorted(f for f in os.listdir(base) if f.endswith(".lua"))
    differing = 0
    for name in names:
        script = os.path.join("test", "compat", name)
        ours = run(minivm, script, args.timeout)
        theirs = run(args.lua, script, args.timeout)
        bad = []
        for i in range(max(len(ours), len(theirs))):
            a = ours[i] if i < len(ours) else "<missing>"
            b = theirs[i] if i < len(theirs) else "<missing>"
            if a != b:
                bad.append((a, b))
        print(f"{name}: {len(bad)} differing lines of {len(theirs)}")
        for a, b in bad:
            print(f"  minivm: {a}")
            print(f"  {args.lua}: {b}")
        differing += len(bad)
    return 1 if differing else 0


if __name__ == "__main__":
    sys.exit(main())
