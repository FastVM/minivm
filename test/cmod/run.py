import argparse
import os
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.join(base, "..", "..")
minivm: str = os.path.join(root, "build", "bin", "minivm")
modules: str = os.path.join(root, "build", "cmod")


def main() -> int:
    ap = argparse.ArgumentParser(description="build the C modules in test/cmod and load them through require")
    ap.add_argument("--cc", default=None, help="compiler for the modules (default: the makefile's CC)")
    args = ap.parse_args()
    make: list[str] = ["make", "-s", "-C", root, "cmod"]
    if args.cc is not None:
        make.append(f"CC={args.cc}")
    built = subprocess.run(make)
    if built.returncode != 0:
        print("cmod: build failed")
        return 1
    proc = subprocess.run([minivm, os.path.join(base, "test.lua"), modules], capture_output=True, timeout=60)
    out: str = proc.stdout.decode(errors="replace").strip()
    if proc.returncode != 0 or out != "ok":
        print(out)
        print(proc.stderr.decode(errors="replace"))
        print(f"cmod: failed (exit {proc.returncode})")
        return 1
    print("cmod: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
