import os
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.join(base, "..", "..")
host: str = os.path.join(root, "build", "embed", "host")


def main() -> int:
    built = subprocess.run(["make", "-s", "-C", root, "embed"])
    if built.returncode != 0:
        print("embed: build failed")
        return 1
    proc = subprocess.run([host], capture_output=True, timeout=60)
    out: str = proc.stdout.decode(errors="replace").strip()
    if proc.returncode != 0 or out != "ok":
        print(out)
        print(proc.stderr.decode(errors="replace"))
        print(f"embed: failed (exit {proc.returncode})")
        return 1
    panic = subprocess.run([host, "panic"], capture_output=True, timeout=60)
    said: str = panic.stdout.decode(errors="replace").strip()
    if panic.returncode != 3 or said != "panic: attempt to index a nil value":
        print(said)
        print(panic.stderr.decode(errors="replace"))
        print(f"embed: panic failed (exit {panic.returncode})")
        return 1
    print("embed: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
