#!/usr/bin/env python3
import os
import subprocess
import sys

base = os.path.dirname(os.path.abspath(__file__))
harness = os.path.join(base, "..", "harness")
minivm = os.path.join(base, "..", "..", "build", "bin", "minivm")


def main():
    exe = minivm
    names = sys.argv[1:]
    if names and names[0] == "--lua":
        exe = names[1]
        names = names[2:]
    for name in names:
        proc = subprocess.run([exe, name], cwd=harness, capture_output=True, timeout=120)
        lines = (proc.stdout + proc.stderr).decode(errors="replace").split("\n")
        showing = False
        for line in lines:
            if line.startswith("not ok"):
                showing = True
                print(name + ": " + line)
            elif showing and line.startswith("#"):
                print("    " + line)
            else:
                showing = False


if __name__ == "__main__":
    main()
