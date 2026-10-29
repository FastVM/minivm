#!/usr/bin/env python3
import os
import subprocess
import sys
from collections import defaultdict

base = os.path.dirname(os.path.abspath(__file__))
minivm = os.path.join(base, "..", "..", "build", "bin", "minivm")
probes = os.path.join(base, "probes")
out_dir = os.path.join(base, "out")


def run_probe(name):
    expected_path = os.path.join(probes, name + ".expected")
    with open(expected_path, "rb") as f:
        expected = f.read()
    try:
        proc = subprocess.run([minivm, name + ".lua"], cwd=probes, capture_output=True, timeout=30)
        got = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired:
        got = b"<timeout>"
    return got == expected, got


def main():
    os.makedirs(out_dir, exist_ok=True)
    names = sorted(
        f[:-4] for f in os.listdir(probes) if f.endswith(".lua")
    )
    areas = defaultdict(lambda: [0, 0])
    rows = []
    for name in names:
        passed, got = run_probe(name)
        area = name.split("_", 1)[0]
        areas[area][0 if passed else 1] += 1
        status = "PASS" if passed else "FAIL"
        got_path = os.path.join(out_dir, name + ".got")
        if passed:
            if os.path.exists(got_path):
                os.remove(got_path)
        else:
            with open(got_path, "wb") as f:
                f.write(got)
        summary = got.decode(errors="replace").split("\n", 1)[0][:60]
        rows.append(f"{area}\t{name}\t{status}\t{summary}")
    with open(os.path.join(out_dir, "results.tsv"), "w") as f:
        f.write("\n".join(rows) + "\n")
    total_pass = 0
    total = 0
    print(f"{'area':<8} {'probes':>6} {'pass':>6} {'fail':>6}")
    for area in sorted(areas):
        passed, failed = areas[area]
        total_pass += passed
        total += passed + failed
        print(f"{area:<8} {passed + failed:>6} {passed:>6} {failed:>6}")
    pct = 100.0 * total_pass / total if total else 0.0
    print(f"TOTAL  {total:>7} {total_pass:>6} {total - total_pass:>6}  ({pct:.1f}%)")
    return 1 if total_pass != total else 0


if __name__ == "__main__":
    sys.exit(main())
