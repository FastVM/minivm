#!/usr/bin/env python3
import os
import re
import shlex
import subprocess
import sys

base = os.path.dirname(os.path.abspath(__file__))
minivm = os.path.join(base, "..", "..", "build", "bin", "minivm")
verbose = False
wrap = shlex.split(os.environ.get("MX_WRAP", ""))
known = {("304-string.t", 43), ("304-string.t", 48), ("309-os.t", 14)}


def run_file(name):
    try:
        proc = subprocess.run(
            wrap + [minivm, name], cwd=base, capture_output=True, timeout=1200 if wrap else 120
        )
        out = proc.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        return None, 0, [], 0
    ok = len(re.findall(r"^ok[ \t]", out, re.M))
    fail = [int(n) for n in re.findall(r"^not ok[ \t]+(\d+)", out, re.M)]
    skip = len(re.findall(r"# [Ss]kip", out))
    plan = re.search(r"^1\.\.(\d+)", out, re.M)
    if verbose:
        showing = False
        for line in out.splitlines():
            if line.startswith("not ok"):
                showing = True
            elif not line.startswith("#"):
                showing = False
            if showing:
                print(f"  {name}: {line}")
        if plan is None:
            print(f"  {name}: exit {proc.returncode}, {len(out)} bytes of output")
            print(out[-4000:])
            print(proc.stderr.decode(errors="replace"))
    return plan, ok, fail, skip


def main():
    global verbose
    files = sys.argv[1:]
    if "-v" in files:
        files.remove("-v")
        verbose = True
    if not files:
        files = sorted(f for f in os.listdir(base) if f.endswith(".t"))
    total_ok = 0
    total_fail = 0
    total_skip = 0
    unexpected = []
    print(f"{'file':<28} {'ok':>6} {'fail':>6} {'skip':>6}")
    for name in files:
        plan, ok, fails, skip = run_file(name)
        if plan is None and ok == 0:
            print(f"{name:<28} {ok:>6} {'?':>6} {'?':>6}  (crash or compile error)")
            total_fail += 1
            unexpected.append(f"{name} crashed")
            continue
        fail = len(fails)
        unexpected.extend(f"{name} #{n}" for n in fails if (name, n) not in known)
        if plan is not None:
            missing = int(plan.group(1)) - ok - fail
            if missing > 0:
                fail += missing
                unexpected.append(f"{name} missing {missing}")
        total_ok += ok
        total_fail += fail
        total_skip += skip
        print(f"{name:<28} {ok:>6} {fail:>6} {skip:>6}")
    print(f"TOTAL ok={total_ok} fail={total_fail} skip={total_skip} unexpected={len(unexpected)}")
    for u in unexpected:
        print(f"  unexpected: {u}")
    return 1 if unexpected else 0


if __name__ == "__main__":
    sys.exit(main())
