import argparse
import os
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
minivm: str = os.path.join(base, "..", "..", "build", "bin", "minivm")
script: str = os.path.join(base, "chunks.lua")


def main() -> int:
    ap = argparse.ArgumentParser(description="load corrupted string.dump output and report crashes")
    ap.add_argument("-n", type=int, default=200, help="number of seeds")
    ap.add_argument("-s", type=int, default=0, help="first seed")
    ap.add_argument("--per", type=int, default=50, help="corrupted chunks per seed")
    ap.add_argument("--timeout", type=float, default=10.0)
    args = ap.parse_args()
    crashes: list[str] = []
    hangs: list[int] = []
    for seed in range(args.s, args.s + args.n):
        try:
            proc = subprocess.run([minivm, script, str(seed), str(args.per)], capture_output=True, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            hangs.append(seed)
            continue
        if proc.returncode != 0:
            tail: str = proc.stderr.decode(errors="replace").strip().split("\n")[-1]
            crashes.append(f"seed={seed} exit={proc.returncode} {tail}")
    for line in crashes:
        print(line)
    if hangs:
        print("hangs:", " ".join(str(s) for s in hangs))
    print(f"seeds={args.n} crashes={len(crashes)} hangs={len(hangs)}")
    return 1 if crashes else 0


if __name__ == "__main__":
    sys.exit(main())
